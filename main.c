#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "archiver.h"
#define BUFFER_SIZE 1024

//
void updateInsertMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) );

//
void updateMoveMember ( int argc, char** argv,
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
            fp = updateInsertMembers;
            oper = insertMember;
            optCountVerify(&optCount);
            break;

        case 'a':
            fp = updateInsertMembers;
            oper = updateMember;
            optCountVerify(&optCount);
            break;

        case 'm':
            argv[0] = optarg;
            fp = updateMoveMember;
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

void updateInsertMembers ( int argc, char** argv,
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

int loadArchiveFromFile ( FILE** f, char* fName, archive_t** a )
{
    *a = allocateArchive();
    if (! (*a))
        return 2;

    // Tentando abrir o a 
    *f = fopen(fName, "r+");
    if (! (*f)) {
        if (errno == EACCES) {
            //fileOperationFailMessage(errno, filename);
            freeArchive(*a);
            return 3;

        // Caso a falha tenha ocorrido por motivo de nao existir algo no caminho
        // pode ser o arquivo ou algum diretorio no caminho
        // se for o arquivo vai tentar criar mais pra frente
        } else if (errno == ENOENT) {
            return 1;
        }

    } else {
        int error = loadArchive (*f, *a);
        if (error != 0) {
            fclose(*f);
            freeArchive(*a);

            //fprintf(stderr, "Falha ao ler dados do arquivo '%s'\n", filename);
            return 3;
        }
    }

    return 0;
}

// optarg is in argv[0]
void updateMoveMember ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    if (argc > 5) {
        fprintf(stderr, "Parametros incorretos\n");
        return;
    }

    FILE* arq;
    archive_t* archive;
    char* filename = argv[optind];

    switch (loadArchiveFromFile(&arq, filename, &archive)){
    case 1:
        fprintf(stderr, "Arquivo inexistente no caminho\n");
        // tenta criar o arquivo
        break;

    case 2:
        fprintf(stderr, "Falha de alocacao de memoria dinamica\n");
        return;

    case 3:
        fprintf(stderr, "Falha ao carregar sessao de diretorio\n");
        return;
    }

    // Caso nao tenha conseguido abrir, e nao tenha conseguido
    if (! arq) {
        freeArchive(archive);
        fileOperationFailMessage(errno, filename);
        exit(2);
    }

    int error = moveMember(arq, archive, argv[0], argv[optind+1]);

    if (error == 0) {
        fprintf(stderr, "Erro ao mover bytes do archive\n");
        freeArchive(archive);
        fclose(arq);

        treatError(error);
    }

    error = writeArchive(arq, archive);
    ftruncate(fileno(arq), ftell(arq));

    freeArchive(archive);
    fclose(arq);

    if (error != 0)
        treatError(error);
}

void extractMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    FILE* arq;
    archive_t* archive;
    char* filename = argv[optind];

    switch (loadArchiveFromFile(&arq, filename, &archive)){
    case 1:
        fprintf(stderr, "Arquivo inexistente no caminho\n");
        return;

    case 2:
        fprintf(stderr, "Falha de alocacao de memoria dinamica\n");
        return;

    case 3:
        fprintf(stderr, "Falha ao carregar sessao de diretorio\n");
        return;
    }

    // Caso nao tenha conseguido abrir, e nao tenha conseguido carregar o archive
    if (! arq) {
        freeArchive(archive);
        fileOperationFailMessage(errno, filename);
        exit(2);
    }

    int error;
    FILE* dest;
    for (unsigned int index = optind+1; index < argc; index++) {
        dest = fopen(argv[index], "w");
        if (! dest) {
            freeArchive(archive);
            fclose(arq);
            fileOperationFailMessage(errno, argv[index]);
            exit(2);
        }
        error = extractMember(arq, dest, argv[index], archive);
        fclose(dest);

        if (error == 0) {
            fprintf(stderr, "%d Erro ao extrair membro do archive\n", error);
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

void removeMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    FILE* arq;
    archive_t* archive;
    char* filename = argv[optind];

    switch (loadArchiveFromFile(&arq, filename, &archive)){
    case 1:
        fprintf(stderr, "Arquivo inexistente no caminho\n");
        return;

    case 2:
        fprintf(stderr, "Falha de alocacao de memoria dinamica\n");
        return;

    case 3:
        fprintf(stderr, "Falha ao carregar sessao de diretorio\n");
        return;
    }

    // Caso nao tenha conseguido abrir, e nao tenha conseguido carregar o archive
    if (! arq) {
        freeArchive(archive);
        fileOperationFailMessage(errno, filename);
        exit(2);
    }

    int error;
    for (unsigned int index = optind+1; index < argc; index++) {
        error = removeMember(arq, archive, argv[index]);

        if (error == 0) {
            fprintf(stderr, "%d Erro ao remover membro do archive\n", error);
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
