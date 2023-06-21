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

void updateAllMembers( int argc, char** argv )
{
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        if (errno == EACCES) {
            fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
            fprintf(stderr, "\tSem permissao para acessar o caminho/arquivo descrito\n");
            exit(2);

        } else if (errno == ENOENT) {
            arq = fopen(filename, "w");
        }
    }

    if (! arq) {
        if (errno == EACCES) {
            fprintf(stderr, "Falha ao abrir o arquivo '%s' para escrita\n", filename);
            fprintf(stderr, "\tSem permissao para acessar o caminho/arquivo descrito\n");
            exit(2);

        } else if (errno == ENOENT) {
            fprintf(stderr, "\tDiretorio ou arquivo inexistente no caminho descrito\n");
            exit (2);
        }
    }



    fclose(arq);
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

    // captura das caracteristicas de um arquivo
    // nome, uid, data de modificacao, permissoes
    // caminho ja esta incluso no filename
    //struct stat s;
    //if(stat(filename, &s) == -1) {
    //    fprintf(stderr, "Falha ao ler metadados do arquivo '%s'\n", filename);
    //    exit(2);
    //}else{
    //    printf("Nome do arquivo-----------------: %s\n", filename);
    //    printf("ID do usuario criador do arquivo: %d\n", s.st_uid);
    //    printf("Data de modificacao do arquivo--: %09ld\n", s.st_mtim.tv_sec);
    //    printf("Permissoes do arquivo-----------: %d\n", s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
    //}

    //system("echo funciona");

    FILE* arq = fopen(filename, "r");
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
        fprintf(stderr, "Falha de alocacao de memoria dinamica\n");
        fclose(arq);
        exit(1);
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
