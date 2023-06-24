#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "archiver.h"
#define BUFFER_SIZE 1024

//
void updateMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) );

//
void moveMember ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) );

//
void extractMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) );

//
void removeMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) );

//
void listMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) );

//
void help ( );

// Encerra o programa se mais de uma opcao foi selecionada.
void optCountVerify ( char* optCount );

int main(int argc, char **argv)
{
    int opt;
    char optCount = 0;
    void (*fp) (int argc, char** argv,
                int (*uF) (FILE* src, FILE* dest, char* srcName, archive_t* a));
    int (*oper) (FILE* src, FILE* dest, char* srcName, archive_t* a);

    if (argc <= 2) {
        fprintf(stderr, "Opcao invalida\n");
        exit(1);
    }		

    oper = NULL;
    while ((opt = getopt(argc, argv, "iam:xrch")) != -1) {
        switch (opt) {
        case 'i':
            fp = updateMembers;
            oper = insertMember;
            optCountVerify(&optCount);
            break;

        case 'a':
            fp = updateMembers;
            oper = updateMember;
            optCountVerify(&optCount);
            break;

        case 'm':
            argv[0] = optarg;
            fp = moveMember;
            optCountVerify(&optCount);
            break;

        case 'x':
            fp = extractMembers;
            optCountVerify(&optCount);
            break;

        case 'r':
            fp = removeMembers;
            optCountVerify(&optCount);
            break;

        case 'c':
            fp = listMembers;
            optCountVerify(&optCount);
            break;

        case 'h':
            fp = help; 
            optCountVerify(&optCount);
            break;

        default:
            fprintf(stderr, "Opcao invalida\n");
            exit(1);
        }		
    }

    fp(argc, argv, oper);

    return 0;
}

void optCountVerify(char* optCount) {
    if (*optCount >= 1) {
        fprintf(stderr, "Opcao invalida\n");
        exit(1);
    }

    (*optCount)++;
}

void fileOperationFailMessage ( int err, char* filename )
{
    fprintf(stderr, 
            "Falha ao abrir o arquivo '%s'\n",
            filename);

    if (errno == EACCES)
        fprintf(stderr, "\tSem permissao para acessar o caminho/arquivo descrito\n");
    else if (errno == ENOENT)
        fprintf(stderr, "\tDiretorio ou arquivo inexistente no caminho descrito\n");
}

void exitFail ( char* message, int code )
{
    fprintf(stderr, "%s\n", message);
    exit(code);
}

void treatError ( int code )
{
    fprintf(stderr, "Erro %d\n", code);
    exit(1);
}

void updateMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    int error;

    archive_t* archive = allocateArchive();
    if (! archive)
        exitFail("Falha de alocacao de memoria dinamica", 1);

    // Tentando abrir o archive 
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        if (errno == EACCES) {
            fileOperationFailMessage(errno, filename);
            freeArchive(archive);
            exit(2);

        // Caso a falha tenha ocorrido por motivo de nao existir algo no caminho
        // tenta criar o arquivo, imanginando que foi passado um diretorio
        // e o archive nao existe nele
        } else if (errno == ENOENT) {
            arq = fopen(filename, "w");
        }

    // Caso nao tenha conseguido abrir, tenta carregar os dados dos membros
    } else {
        error = loadArchive (arq, archive);
        if (error != 0) {
            fclose(arq);
            freeArchive(archive);

            fprintf(stderr, "Falha ao ler dados do arquivo '%s'\n", filename);
            exit(3);
        }
    }

    // Caso nao tenha conseguido abrir, e nao tenha conseguido
    if (! arq) {
        freeArchive(archive);
        fileOperationFailMessage(errno, filename);
        exit(2);
    }

    FILE* src;
    for (unsigned int index = optind+1; index < argc; index++) {
        src = fopen(argv[index], "r");
        if (! src) {
            freeArchive(archive);
            fclose(arq);
            fileOperationFailMessage(errno, argv[index]);
            exit(2);
        }

        error = oper(src, arq, argv[index], archive);
        fclose(src);

        if (error != 0) {
            fprintf(stderr, "Erro ao inserir membro no archive\n");
            freeArchive(archive);
            fclose(arq);

            treatError(error);
        }
    }

    error = writeArchive(arq, archive);
    ftruncate(fileno(arq), ftell(arq));

    freeArchive(archive);
    fclose(arq);

    if (error != 0)
        treatError(error);
}

// optarg is in argv[0]
void moveMember ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    return;
}

void extractMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    return;
}

void removeMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    return;
}

void listMembers( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura\n", filename);
        if (errno == EACCES)
            fprintf(stderr, "\tSem permissao para acessar o caminho/arquivo descrito\n");
        else if (errno == ENOENT)
            fprintf(stderr, "\tDiretorio ou arquivo inexistente no caminho descrito\n");
        
        exit(2);
    }

    archive_t* archive = allocateArchive();
    if (! archive) {
        fclose(arq);
        exitFail("Falha de alocacao de memoria dinamica", 1);
    }

    int error;
    error = loadArchive (arq, archive);
    if (error != 0) {
        fprintf(stderr, "Falha ao ler dados do arquivo '%s'\n", filename);
        fclose(arq);
        freeArchive(archive);
        exit(3);
    }

    printArchive(archive); 

    fclose(arq);
    freeArchive(archive);
}

void help ( )
{
    return;
}
