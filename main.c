#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "archiver.h"

#define BUFFER_SIZE 1024

//
void updateAllMembers ( int argc, char** argv );

//
void updateNewMembers ( int argc, char** argv );

//
void moveMember ( int argc, char** argv );

//
void extractMembers ( int argc, char** argv );

//
void removeMembers ( int argc, char** argv );

//
void listMembers ( int argc, char** argv );

//
void help ( );

// Encerra o programa se mais de uma opcao foi selecionada.
void optCountVerify ( char* optCount );

int main(int argc, char **argv)
{
    int opt;
    char optCount = 0;
    void (*fp) (int argc, char** argv);

    if (argc <= 2) {
        fprintf(stderr, "Opcao invalida\n");
        exit(1);
    }		

    while ((opt = getopt(argc, argv, "iam:xrch")) != -1) {
        switch (opt) {
        case 'i':
            fp = updateAllMembers;
            optCountVerify(&optCount);
            break;

        case 'a':
            fp = updateNewMembers;
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

    fp(argc, argv);

    return 0;
}

void optCountVerify(char* optCount) {
    if (*optCount >= 1) {
        fprintf(stderr, "Opcao invalida\n");
        exit(1);
    }

    (*optCount)++;
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

void updateAllMembers( int argc, char** argv )
{
    int error;

    archive_t* archive = allocateArchive();
    if (! archive)
        exitFail("Falha de alocacao de memoria dinamica", 1);

    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        if (errno == EACCES) {
            fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
            freeArchive(archive);

            exitFail("\tSem permissao para acessar o caminho/arquivo descrito", 2);

        } else if (errno == ENOENT) {
            arq = fopen(filename, "w");
        }
    } else {
        error = loadArchive (arq, archive);
        if (error != 0) {
            fclose(arq);
            freeArchive(archive);

            fprintf(stderr, "Falha ao ler dados do arquivo '%s'\n", filename);
            exit(3);
        }
    }

    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para escrita\n", filename);
        freeArchive(archive);

        if (errno == EACCES) 
            exitFail("\tSem permissao para acessar o caminho/arquivo descrito\n", 2);
        else if (errno == ENOENT)
            exitFail("\tDiretorio ou arquivo inexistente no caminho descrito\n", 4);
    }

    FILE* src;
    for (unsigned int index = optind+1; index < argc; index++) {
        src = fopen(argv[index], "r");
        if (! src) {
            fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura\n", argv[index]);
            if (errno == EACCES)
                fprintf(stderr, "\tSem permissao para acessar o caminho/arquivo descrito\n");
            else if (errno == ENOENT)
                fprintf(stderr, "\tDiretorio ou arquivo inexistente no caminho descrito\n");

            freeArchive(archive);
            fclose(arq);
            exit(2);
        }

        error = insertMember(src, arq, argv[index], archive);
        fclose(src);
        if (error != 0) {
            fprintf(stderr, "Erro ao inserir membro no archive\n");
            freeArchive(archive);
            fclose(arq);

            treatError(error);
        }

    }

    error = writeArchive(arq, archive);

    freeArchive(archive);
    fclose(arq);

    if (error != 0)
        treatError(error);
}

void updateNewMembers ( int argc, char** argv )
{
    return;
}

// optarg is in argv[0]
void moveMember ( int argc, char** argv )
{
    return;
}

void extractMembers ( int argc, char** argv )
{
    return;
}

void removeMembers ( int argc, char** argv )
{
    return;
}

void listMembers( int argc, char** argv )
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
