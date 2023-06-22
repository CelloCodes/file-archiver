#include <stdio.h>
#include <stdlib.h>

#include "archiver.h"

archive_t* allocateArchive ( )
{
    archive_t* a = malloc(sizeof(archive_t));
    if (! a)
        return NULL;

    a->numMembers = 0;

    a->memberTree = NULL;

    a->firstInOrder = NULL;
    a->lastInOrder = NULL;

    return a;
}

int readMemberData ( FILE* src, memberData_t *m )
{
    if (fread(&(m->sizeofName), sizeof(m->sizeofName), 1, src) != 1)
        return 0;

    m->name = malloc(sizeof(char) * m->sizeofName);
    if (! m->name)
        return 0;

    if (fread(m->name, m->sizeofName, 1, src) != 1)
        return 0;

    if (fread(&(m->permission), sizeof(m->permission), 1, src) != 1)
        return 0;

    if (fread(&(m->order), sizeof(m->order), 1, src) != 1)
        return 0;

    if (fread(&(m->size), sizeof(m->size), 1, src) != 1)
        return 0;

    if (fread(&(m->UID), sizeof(m->UID), 1, src) != 1)
        return 0;

    if (fread(&(m->modDate), sizeof(m->modDate), 1, src) != 1)
        return 0;

    return 1;
}

void insertInOrder ( FILE* src, archive_t* a, memberData_t* m )
{
    memberData_t* aux = a->firstInOrder;
    while((aux) && (aux->nextInOrder) && (aux->order < m->order))
        aux = aux->nextInOrder;

    if (! aux) {
        a->firstInOrder = m;
        a->lastInOrder = m;

    } else if (! aux->nextInOrder) {
        aux->nextInOrder = m;
        m->previousInOrder = aux;

        a->lastInOrder = m;

    } else {
        if (aux->previousInOrder) {
            m->previousInOrder = aux->previousInOrder;
            m->previousInOrder->nextInOrder = m;
            aux->previousInOrder = m;

        } else {
            aux->previousInOrder = m;
            m->nextInOrder = aux;

            a->firstInOrder = m;
        }
    }
}

int loadArchive ( FILE* src, archive_t* a )
{
    rewind(src);

    unsigned int posArchive;
    if (fread(&posArchive, sizeof(posArchive), 1, src) != 1)
        return 1;

    fseek(src, posArchive, SEEK_SET);
    if (fread(&(a->numMembers), sizeof(a->numMembers), 1, src) != 1)
        return 1;

    memberData_t* member;
    for (unsigned int i = 0; i < a->numMembers; a++) {
        member = malloc(sizeof(memberData_t));
        if (! member)
            return 2;

        if (! readMemberData(src, member)) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 2;
        }

        if (! treeInsert(&(a->memberTree), member)) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 2;
        }

        insertInOrder(src, a, member);
    }

    return 0;
}

// verifica se o membro esta contido no archive
// se estiver executa processo 1
// se nao estiver insere ao final
int insertMember ( FILE* src, FILE* dest, char* srcName, archive_t* a )
{
    return 0;
}

// verifica se o membro esta contido no archive
// se estiver verifica se a data de modificacao mudou
// se tiver mudado executa processo 1
// se nao estiver no arquivo insere ao final
int updateMember ( FILE* src, FILE* dest, char* srcName, archive_t* a )
{
    return 0;
}

void printArchive ( archive_t* a )
{
    return;
}

void freeArchive ( archive_t* a )
{
    freeTree(a->memberTree);
    free(a);
}
