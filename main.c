#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
        exit(2);
    }

    fclose(arq);
}

void updateNewMembers ( int argc, char** argv )
{
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
        exit(2);
    }

    fclose(arq);
}

// optarg is in argv[0]
void moveMember ( int argc, char** argv )
{
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
        exit(2);
    }

    fclose(arq);
}

void extractMembers ( int argc, char** argv )
{
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
        exit(2);
    }

    fclose(arq);
}

void removeMembers ( int argc, char** argv )
{
    char* filename = argv[optind];
    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
        exit(2);
    }

    fclose(arq);
}

void listMembers( int argc, char** argv )
{
    char* filename = argv[optind];
    //struct stat* s = malloc(sizeof(struct stat));

    //if(! s) {
    //    fprintf(stderr, "Falha ao alocar memoria dinamica\n");
    //    free(s);
    //    exit(3);
    //}

    struct stat s;
    
    if(stat(filename, &s) == -1) {
        fprintf(stderr, "Falha ao ler metadados do arquivo '%s'\n", filename);
        //exit(2);
    }else{
        printf("Nome do arquivo-----------------: %s\n", filename);
        printf("ID do usuario criador do arquivo: %d\n", s.st_uid);
        printf("Data de modificacao do arquivo--: %09ld\n", s.st_mtim.tv_sec);
        printf("Modo do arquivo-----------------: %d\n", s.st_mode);
        printf("Permissoes do arquivo-----------: %d\n", s.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
    }

    //system("echo funciona");


    FILE* arq = fopen(filename, "r+");
    if (! arq) {
        fprintf(stderr, "Falha ao abrir o arquivo '%s' para leitura e escrita\n", filename);
        if (errno == EACCES)
            fprintf(stderr, "\tSem permissao\n");
        else if (errno == ENOENT)
            fprintf(stderr, "\tDiretorio ou arquivo inexistente\n");
        
        exit(2);
    }

    printf("Abriu o arquivo com sucesso\n");

    //tipo* archive;
    //archive = readArchive(arq);
    //if (archive == NULL) {
    //    fprintf(stderr, "Falha ao ler arquivo %s\n", argv[1]);
    //    return;
    //}

    fclose(arq);
}

void help ( )
{
    return;
}
