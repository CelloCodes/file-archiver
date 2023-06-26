#ifndef ARCHIVER_H_
#define ARCHIVER_H_

#include "avl.h"

// ideia: escrever os nodos da arvore em pre ordem
// dessa forma, quando forem lidos do arquivo
// nenhuma rotacao sera necessaria pois serao colocados
// na sua posicao ja calculada

typedef struct {
    long numMembers;

    treeNode_t* memberTree;

    memberData_t* firstInOrder;
    memberData_t* lastInOrder;

} archive_t;

archive_t* allocateArchive ( );

int loadArchive ( FILE* src, archive_t* a );

int insertMember ( FILE* src, FILE* dest, char* srcName, archive_t* a );

int updateMember ( FILE* src, FILE* dest, char* srcName, archive_t* a );

int moveMember ( FILE* dest, archive_t* a, char* m1Name, char* m2Name );

int removeMember ( FILE* dest, archive_t* a, char* m1Name );

int extractMember ( FILE* src, FILE* dest, char* name, archive_t* a );

int extractAllMembers ( FILE* src, archive_t* a );

int writeArchive ( FILE* dest, archive_t* a );

void printArchive ( archive_t* a );

void freeArchive ( archive_t* a );

#endif
