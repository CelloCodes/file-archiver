#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "archiver.h"

#define BUFFER_SIZE 1024

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

void freeArchive ( archive_t* a )
{
    freeTree(a->memberTree);
    free(a);
}

memberData_t* allocateMember ( unsigned short sizeofName, char* name )
{
    memberData_t* m = malloc(sizeof(memberData_t));
    if (! m)
        return NULL;

    m->name = malloc(sizeof(char) * sizeofName);
    if (! m->name) {
        free(m);
        return NULL;
    }

    for (unsigned short i = 0; i < sizeofName; i++)
        m->name[i] = name[i];

    m->sizeofName = sizeofName;
    m->previousInOrder = NULL;
    m->nextInOrder = NULL;

    return m;
}

void freeMember ( memberData_t* m )
{
    if (m) {
        free(m->name);
        free(m);
    }
}

void fillMemberStat ( memberData_t* m, struct stat* s )
{
    m->permission  = s->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    m->size        = s->st_size;
    m->UID         = s->st_uid;
    m->modDate     = s->st_mtim.tv_sec;
}

unsigned int getFirstFreeByte ( archive_t* a )
{
    if (a->lastInOrder)
        return (a->lastInOrder->position + a->lastInOrder->size);
    return 0;
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

void insertInOrder ( archive_t* a, memberData_t* m )
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

    a->numMembers++;
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
            freeMember(member);
            freeTree(a->memberTree);
            return 2;
        }

        if (! treeInsert(&(a->memberTree), member)) {
            freeMember(member);
            freeTree(a->memberTree);
            return 2;
        }

        insertInOrder(a, member);
    }

    return 0;
}

// no meio do caminho de uma escrita pode ocorrer erro
// a boa pratica deve ser criar um arquivo novo para evitar corromper dados
int pasteBinary (FILE* src, FILE* dest, unsigned long start, unsigned long bytes )
{
    if (fseek(dest, start, SEEK_SET) == -1)
        return 0;

    void* buffer = malloc(BUFFER_SIZE);
    if (! buffer)
        return 0;

    while (bytes >= BUFFER_SIZE) {
        if (fread(buffer, BUFFER_SIZE, 1, src) != 1) {
            free(buffer);
            return 0;
        }

        if (fwrite(buffer, BUFFER_SIZE, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        bytes -= BUFFER_SIZE;
    }

    if (bytes > 0) {
        if (fread(buffer, bytes, 1, src) != 1) {
            free(buffer);
            return 0;
        }

        if (fwrite(buffer, bytes, 1, dest) != 1) {
            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;
}

int adjustRight ( FILE* dest, long stop, unsigned long bytes)
{
    if (fseek(dest, 0, SEEK_END) == -1)
        return 0;

    void* buffer = malloc(BUFFER_SIZE);
    if (! buffer)
        return 0;

    long curPos = ftell(dest);
    if (curPos < 0) {
        free(buffer);
        return 0;
    }

    while ((curPos - BUFFER_SIZE) >= stop) {
        if (fseek(dest, -BUFFER_SIZE, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fread(buffer, BUFFER_SIZE, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        if (fseek(dest, bytes, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fwrite(buffer, BUFFER_SIZE, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        if (fseek(dest, - bytes - BUFFER_SIZE, curPos) == -1) {
            free(buffer);
            return 0;
        }

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buffer);
            return 0;
        }
    }

    if (curPos > stop) {
        long toRead = curPos - stop;
        if (fseek(dest, stop - curPos, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fread(buffer, toRead, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        if (fseek(dest, curPos - toRead + bytes, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fwrite(buffer, toRead, 1, dest) != 1) {
            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;

}

// adaptar
int adjustRight ( FILE* dest, long stop, unsigned long bytes)
{
    if (fseek(dest, 0, SEEK_END) == -1)
        return 0;

    void* buffer = malloc(BUFFER_SIZE);
    if (! buffer)
        return 0;

    long curPos = ftell(dest);
    if (curPos < 0) {
        free(buffer);
        return 0;
    }

    while ((curPos - BUFFER_SIZE) >= stop) {
        if (fseek(dest, -BUFFER_SIZE, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fread(buffer, BUFFER_SIZE, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        if (fseek(dest, bytes, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fwrite(buffer, BUFFER_SIZE, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        if (fseek(dest, - bytes - BUFFER_SIZE, curPos) == -1) {
            free(buffer);
            return 0;
        }

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buffer);
            return 0;
        }
    }

    if (curPos > stop) {
        long toRead = curPos - stop;
        if (fseek(dest, stop - curPos, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fread(buffer, toRead, 1, dest) != 1) {
            free(buffer);
            return 0;
        }

        if (fseek(dest, curPos - toRead + bytes, curPos) == -1) {
            free(buffer);
            return 0;
        }

        if (fwrite(buffer, toRead, 1, dest) != 1) {
            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;

}

// verifica se o membro esta contido no archive
// se estiver executa processo 1
// se nao estiver insere ao final
int insertMember ( FILE* src, FILE* dest, char* srcName, archive_t* a )
{
    unsigned int nameSize = 0;
    for (; srcName[nameSize] != '\0'; nameSize++);

    if (nameSize >= 255)
        return 3;

    memberData_t* member = allocateMember(nameSize, srcName);
    if (! member)
        return 2;

    struct stat s;
    if(stat(srcName, &s) == -1) {
        freeMember(member);
        return 4;
    }

    fillMemberStat(member, &s);
    member->order = a->numMembers;

    unsigned int pastePosition;
    long sizeDiff;
    treeNode_t* tn = treeSearch(a->memberTree, member);
    if (tn) {
        memberData_t* oldMember = tn->key;
        sizeDiff = s.st_size - oldMember->size;

        if (sizeDiff > 0) {
            if (! adjustRight(dest, (long) (oldMember->position + oldMember->size), sizeDiff)) {
                freeMember(member);
                return 5;
            }
        } else if (sizeDiff < 0) {
            if (! adjustLeft(dest, oldMember->position + oldMember->size, sizeDiff)) {
                freeMember(member);
                return 5;
            }
        }

        fillMemberStat(oldMember, &s);

        pastePosition = oldMember->position;

        freeMember(member);

    } else {
        if (! treeInsert(&(a->memberTree), member)) {
            freeMember(member);
            return 2;
        }

        pastePosition = getFirstFreeByte(a);

        fillMemberStat(member, &s);
        member->position = pastePosition;

        insertInOrder(a, member);
    }

    if (! pasteBinary(src, dest, pastePosition, s.st_size))
        return 5;

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
