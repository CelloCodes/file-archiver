#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

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
            return 0;
        }		
    }

    if ((argc <= 2) && (fp != help)) {
        fprintf(stderr, "Opcao invalida\n");
        return 1;
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

char* createTmpCopy ( FILE* f, char* fName )
{
    char* name = malloc(strlen(fName) + 5);
    if (! name)
        return NULL;

    int i;
    for (i = 0; i < strlen(fName); i++)
        name[i] = fName[i];

    name[i++] = '.';
    name[i++] = 't';
    name[i++] = 'm';
    name[i++] = 'p';
    name[i++] = '\0';

    FILE* copy = fopen(name, "w");
    if (! copy) {
        free(name);
        return NULL;
    }

    void* buf = malloc(BUFFER_SIZE);
    if (! buf) {
        free(name);
        fclose(copy);
        return NULL;
    }

    struct stat s;
    stat(fName, &s);

    long pos = ftell(f);
    fseek(f, 0, SEEK_SET);
    long tamRead = 0;
    while (tamRead + BUFFER_SIZE <= s.st_size) {
        fread(buf, BUFFER_SIZE, 1, f);
        fwrite(buf, BUFFER_SIZE, 1, copy);
        tamRead += BUFFER_SIZE;
    }

    if (tamRead < s.st_size) {
        fread(buf, s.st_size - tamRead, 1, f);
        fwrite(buf, s.st_size - tamRead, 1, copy);
    }

    fseek(f, pos, SEEK_SET);

    free(buf);
    fclose(copy);

    return name;
}

void revertToCopy ( char* origName, char* copyName )
{
    if ((remove(origName)) || (rename(copyName, origName))) {
        fprintf(stderr, "Falha ao reverter para copia temporaria do archive");
        fprintf(stderr, " Archive pode estar corrompido.");
    }

    fprintf(stderr, "Falha na operacao. Archive restaurado de copia temporaria.\n");
    free(copyName);
}

int loadArchiveFromFile ( FILE** f, char* fName, archive_t** a )
{
    *a = allocateArchive();
    if (! (*a))
        return 2;

    // Tentando abrir o a 
    *f = fopen(fName, "r+");
    if (! (*f)) {
        freeArchive(*a);
        if (errno == EACCES) {
            fileOperationFailMessage(errno, fName);
            return 3;

        // Caso a falha tenha ocorrido por motivo de nao existir algo no caminho
        // pode ser o arquivo ou algum diretorio no caminho
        // se for o arquivo vai tentar criar
        } else if (errno == ENOENT) {
            return 1;
        }

    } else {
        if (loadArchive (*f, *a)) {
            fclose(*f);
            freeArchive(*a);
            return 3;
        }
    }

    return 0;
}

int createArchiveFile ( FILE** f, char* fName, archive_t** a )
{
    *a = allocateArchive();
    if (! (*a))
        return 0;

    *f = fopen(fName, "w+");
    if (! (*f)) {
        freeArchive(*a);
        return 0;
    }

    return 1;
}

void updateInsertMembers ( int argc, char** argv,
            int (*oper) ( FILE* src, FILE* dest, char* srcName, archive_t* a ) )
{
    int error;

    FILE* arq;
    archive_t* archive;
    char* filename = argv[optind];

    switch (loadArchiveFromFile(&arq, filename, &archive)) {
    case 1:
        if (! createArchiveFile(&arq, filename, &archive)) {
            fprintf(stderr, "Falha ao criar o arquivo %s\n", filename);
            exit(1);
        }
        break;

    case 2:
        fprintf(stderr, "Falha de alocacao de memoria dinamica\n");
        exit(1);

    case 3:
        fprintf(stderr, "Falha ao carregar sessao de diretorio\n");
        exit(1);
    }

    char* copyName = createTmpCopy(arq, filename);
    if (! copyName) {
        fprintf(stderr, "Falha ao criar copia temporaria para edicao do arquivo\n");
        freeArchive(archive);
        fclose(arq);
        exit(1);
    }

    FILE* src;
    for (unsigned int index = optind+1; index < argc; index++) {
        src = fopen(argv[index], "r");
        if (! src) {
            freeArchive(archive);
            fclose(arq);
            fileOperationFailMessage(errno, argv[index]);
            revertToCopy(filename, copyName);
            exit(1);
        }

        error = oper(src, arq, argv[index], archive);
        fclose(src);

        if (error) {
            fprintf(stderr, "Erro ao inserir membro no archive\n");
            freeArchive(archive);
            fclose(arq);
            revertToCopy(filename, copyName);
            exit(1);
        }
    }

    error = writeArchive(arq, archive);
    ftruncate(fileno(arq), ftell(arq));

    freeArchive(archive);
    fclose(arq);

    if (error) {
        revertToCopy(filename, copyName);
        exit(1);
    }

    remove(copyName);
    free(copyName);
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
        exit(1);

    case 2:
        fprintf(stderr, "Falha de alocacao de memoria dinamica\n");
        exit(1);

    case 3:
        fprintf(stderr, "Falha ao carregar sessao de diretorio\n");
        exit(1);
    }

    char* copyName = createTmpCopy(arq, filename);
    if (! copyName) {
        fprintf(stderr, "Falha ao criar copia temporaria para edicao do arquivo\n");
        freeArchive(archive);
        fclose(arq);
        exit(1);
    }

    int error = moveMember(arq, archive, argv[0], argv[optind+1]);

    if (error) {
        fprintf(stderr, "Erro ao mover bytes do archive\n");
        freeArchive(archive);
        fclose(arq);
        revertToCopy(filename, copyName);
        exit(1);
    }

    error = writeArchive(arq, archive);
    ftruncate(fileno(arq), ftell(arq));

    freeArchive(archive);
    fclose(arq);

    if (error) {
        revertToCopy(filename, copyName);
        exit(1);
    }

    remove(copyName);
    free(copyName);
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

    int error;
    if (argc == 3) {
        error = extractAllMembers(arq, archive);
        if (error) {
            fprintf(stderr, "%d Erro ao extrair membro do archive\n", error);
            freeArchive(archive);
            fclose(arq);
            exit(1);
        }
    }

    FILE* dest;
    for (unsigned int index = optind+1; index < argc; index++) {
        dest = fopen(argv[index], "w");
        if (! dest) {
            freeArchive(archive);
            fclose(arq);
            fileOperationFailMessage(errno, argv[index]);
            exit(1);
        }
        error = extractMember(arq, dest, argv[index], archive);
        fclose(dest);

        if (error) {
            fprintf(stderr, "%d Erro ao extrair membro do archive\n", error);
            freeArchive(archive);
            fclose(arq);
            exit(1);
        }
    }

    freeArchive(archive);
    fclose(arq);
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

    char* copyName = createTmpCopy(arq, filename);
    if (! copyName) {
        fprintf(stderr, "Falha ao criar copia temporaria para edicao do arquivo\n");
        freeArchive(archive);
        fclose(arq);
        exit(1);
    }

    int error;
    for (unsigned int index = optind+1; index < argc; index++) {
        error = removeMember(arq, archive, argv[index]);

        if (error) {
            fprintf(stderr, "%d Erro ao remover membro do archive\n", error);
            freeArchive(archive);
            fclose(arq);
            revertToCopy(filename, copyName);
            exit(1);
        }
    }

    error = writeArchive(arq, archive);
    ftruncate(fileno(arq), ftell(arq));

    freeArchive(archive);
    fclose(arq);

    if (error) {
        revertToCopy(filename, copyName);
        exit(1);
    }

    remove(copyName);
    free(copyName);
}

void listMembers( int argc, char** argv,
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

    printArchive(archive); 

    fclose(arq);
    freeArchive(archive);
}

void help ( )
{
    printf("Meio de uso: vina++ <opção> <archive> [membro1 membro2 ...]\n");
    printf("Opções:\n");
    printf("\t-i: Inserir membros. Caso já existam são sobrescritos\n");
    printf("\t-a: inserir membros. Caso já existam são sobrescritos se tiverem sido alterados\n");
    printf("\t-m: target: Move o membro target para depois do membro passado como parâmetro\n");
    printf("\t-x: Extrair membros. Se nenhum parâmetro for passado extrai todos\n");
    printf("\t-r: Remover membros\n");
    printf("\t-c: Listar membros\n");
    printf("\t-h: Comando de ajuda\n");
}
