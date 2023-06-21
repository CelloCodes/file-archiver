#ifndef ARCHIVER_H_
#define ARCHIVER_H_

#include "avl.h"

// ideia: escrever os nodos da arvore em pre ordem
// dessa forma, quando forem lidos do arquivo
// nenhuma rotacao sera necessaria pois serao colocados
// na sua posicao ja calculada

typedef struct {
    unsigned int numMembers;

    treeNode_t* memberTree;

} archive_t;

archive_t* allocateArchive ( );

int loadArchive ( FILE* src, archive_t* a );

void printArchive ( archive_t* a );

void freeArchive ( archive_t* a );

#endif
