#ifndef ARCHIVER_H_
#define ARCHIVER_H_

typedef struct {
    unsigned int numMembers;

    // ideia: primeiro char de cada nome ser o tamanho do nome
    // dessa forma le-se o tamanho do nome, aloca-se espaco para ele
    // e entao le-se o nome, cujo tamanho ja e conhecido
    // O tamanho+1 deve ser alocado devido ao caractere nulo ao final
    char** memberNames;

    unsigned short* memberPermissions;

    unsigned int* memberPositions;
    unsigned int* memberSizes;
    unsigned int* memberUIDs;

    unsigned long* memberModDates;

} archive_t;

archive_t* loadArchive ( FILE* src );

#endif
