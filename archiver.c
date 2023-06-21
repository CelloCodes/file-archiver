#include <stdio.h>
#include <stdlib.h>

#include "archiver.h"

archive_t* allocateArchive ( )
{
    archive_t* a = malloc(sizeof(archive_t));
    if (! a)
        return NULL;

    a->memberTree = NULL;

    return a;
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

        if (fread(&(member->sizeofName), sizeof(member->sizeofName), 1, src) != 1) {
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        member->name = malloc(sizeof(char) * member->sizeofName);
        if (! member->name) {
            free(member);
            freeTree(a->memberTree);
            return 2;
        }

        if (fread(member->name, member->sizeofName, 1, src) != 1) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        if (fread(&(member->permission), sizeof(member->permission), 1, src) != 1) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        if (fread(&(member->order), sizeof(member->order), 1, src) != 1) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        if (fread(&(member->size), sizeof(member->size), 1, src) != 1) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        if (fread(&(member->UID), sizeof(member->UID), 1, src) != 1) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        if (fread(&(member->modDate), sizeof(member->modDate), 1, src) != 1) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 1;
        }

        if (! treeInsert(&(a->memberTree), member)) {
            free(member->name);
            free(member);
            freeTree(a->memberTree);
            return 2;
        }
    }

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
