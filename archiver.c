#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>

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

int getFirstFreeByte ( archive_t* a )
{
    if (a->lastInOrder)
        return (a->lastInOrder->position + a->lastInOrder->size);

    return sizeof(int);
}

int readMemberData ( FILE* src, memberData_t *m )
{
    if (fread(&(m->sizeofName), sizeof(m->sizeofName), 1, src) != 1)
        return 0;

    m->name = malloc(sizeof(char) * m->sizeofName);
    if (! m->name)
        return 0;

    if (fread(m->name, sizeof(char) * m->sizeofName, 1, src) != 1)
        return 0;

    if (fread(&(m->permission), sizeof(m->permission), 1, src) != 1)
        return 0;

    if (fread(&(m->order), sizeof(m->order), 1, src) != 1)
        return 0;

    if (fread(&(m->position), sizeof(m->position), 1, src) != 1)
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
        if (aux->order < m->order) {
            aux->nextInOrder = m;
            m->previousInOrder = aux;

            a->lastInOrder = m;
        } else {
            if (aux->previousInOrder) {
                m->previousInOrder = aux->previousInOrder;
                m->previousInOrder->nextInOrder = m;

                m->nextInOrder = aux;
                aux->previousInOrder = m;

            } else {
                aux->previousInOrder = m;
                m->nextInOrder = aux;

                a->firstInOrder = m;
            }
        }

    } else {
        if (aux->previousInOrder) {
            m->previousInOrder = aux->previousInOrder;
            m->previousInOrder->nextInOrder = m;

            m->nextInOrder = aux;
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

    if (fseek(src, posArchive, SEEK_SET) == -1)
        return 1;

    if (fread(&(a->numMembers), sizeof(a->numMembers), 1, src) != 1)
        return 1;

    memberData_t* member;
    for (long i = 0; i < a->numMembers; i++) {
        member = malloc(sizeof(memberData_t));
        if (! member)
            return 2;

        member->name = NULL;
        member->nextInOrder = NULL;
        member->previousInOrder = NULL;
        int erro = readMemberData(src, member);
        if (! erro) {
            freeMember(member);
            return 2;
        }

        if (! treeInsert(&(a->memberTree), member)) {
            freeMember(member);
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

int adjustBinaryRight ( FILE* dest, long stop, unsigned long bytes )
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

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buffer);
            return 0;
        }

        if (    (fread(buffer, BUFFER_SIZE, 1, dest) != 1)  || 
                (fseek(dest, bytes, curPos) == -1)          ||
                (fwrite(buffer, BUFFER_SIZE, 1, dest) != 1) ||
                (fseek(dest, 0, curPos) == -1)              ){

            free(buffer);
            return 0;
        }
    }

    if (curPos > stop) {
        long toRead = curPos - stop;
        if (    (fseek(dest, stop, SEEK_SET) == -1)         ||
                (fread(buffer, toRead, 1, dest) != 1)       ||
                (fseek(dest, stop + bytes, SEEK_SET) == -1) ||
                (fwrite(buffer, toRead, 1, dest) != 1)      ){

            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;
}

int adjustBinaryLeft ( FILE* dest, long start, unsigned long bytes)
{
    if (fseek(dest, 0, SEEK_END) == -1)
        return 0;

    long stop = ftell(dest);
    if (stop < 0)
        return 0;

    if (fseek(dest, start, SEEK_SET) == -1)
        return 0;

    long curPos = ftell(dest);
    if (curPos < 0)
        return 0;

    void* buffer = malloc(BUFFER_SIZE);
    if (! buffer)
        return 0;

    while ((curPos + BUFFER_SIZE) < stop) {
        if (    (fread(buffer, BUFFER_SIZE, 1, dest) != 1)  || 
                (fseek(dest, -bytes, curPos) == -1)         ||
                (fwrite(buffer, BUFFER_SIZE, 1, dest) != 1) ||
                (fseek(dest, bytes, SEEK_CUR) == -1)        ){

            free(buffer);
            return 0;
        }

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buffer);
            return 0;
        }
    }

    if (curPos - stop) {
        long toRead = stop - curPos;
        if (    (fread(buffer, toRead, 1, dest) != 1)       ||
                (fseek(dest, -bytes, curPos) == -1)         ||
                (fwrite(buffer, toRead, 1, dest) != 1)      ){

            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;
}

void adjustPositions ( memberData_t* start, long sizeDiff )
{
    while (start) {
        start->position += sizeDiff;
        start = start->nextInOrder;
    }
}

int insertMember ( FILE* src, FILE* dest, char* srcName, archive_t* a )
{
    int nameSize = 0;
    for (; srcName[nameSize] != '\0'; nameSize++);
    nameSize++;

    if (nameSize >= 255)
        return 3;

    memberData_t* member = allocateMember((unsigned short) nameSize, srcName);
    if (! member)
        return 2;

    struct stat s;
    if(stat(srcName, &s) == -1) {
        freeMember(member);
        return 4;
    }

    fillMemberStat(member, &s);
    member->order = a->numMembers;

    long pastePosition;
    long sizeDiff;
    treeNode_t* tn = treeSearch(a->memberTree, member);
    if (tn) {
        memberData_t* oldMember = tn->key;
        sizeDiff = s.st_size - oldMember->size;

        if (oldMember->modDate == member->modDate) {
            freeMember(member);
            return 0;
        }

        if (sizeDiff > 0) {
            if (! adjustBinaryRight(dest, oldMember->position + oldMember->size, sizeDiff)) {
                freeMember(member);
                return 5;
            } else {
                adjustPositions(oldMember->nextInOrder, sizeDiff) ;
            }
        } else if (sizeDiff < 0) {
            if (! adjustBinaryLeft(dest, oldMember->position + oldMember->size, sizeDiff)) {
                freeMember(member);
                return 5;
            } else {
                adjustPositions(oldMember->nextInOrder, sizeDiff) ;
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

        a->numMembers++;
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

int writeArchive ( FILE* dest, archive_t* a )
{
    if (fseek(dest, 0, SEEK_SET) == -1)
        return 0;

    int pos = getFirstFreeByte(a);
    if (fwrite(&pos, sizeof(int), 1, dest) != 1)
        return 0;

    long numNodes = 0;
    if (a->lastInOrder)
        numNodes = a->lastInOrder->order + 1;

    return treeWriteBFS(dest, a->memberTree, numNodes, pos);
}

void printArchive ( archive_t* a )
{
    printf("-/-/-/-/-/-/-/-/--/-/-/-/-/-/-/-/-/-/-/-/-/-/\n");
    printf("Numero de Membros: %ld\n", a->numMembers);
    printf("-/-/-/-/-/-/-/-/--/-/-/-/-/-/-/-/-/-/-/-/-/-/\n");

    memberData_t* m = a->firstInOrder;

    char perm[10];
    perm[9] = '\0';
    char empty[] = {'-', '-', '-', '-', '-', '-', '-', '-', '-'};
    char full[] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};
    int masks[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};

    char date[80];
    struct tm ts;
    
    short i;
    while (m) {
        for(i = 0; i < 9; i++)
            (m->permission & masks[i]) ? (perm[i] = full[i]) : (perm[i] = empty[i]);
        ts = *localtime(&m->modDate);
        strftime(date, sizeof(date), "%d-%m-%Y %H:%M:%S %Z", &ts);
            
        printf("Membro %2ld: %s\n", m->order, m->name);
        printf("\tUID----------------: %d\n", m->UID);
        printf("\tPermissoes---------: %s\n", perm);
        printf("\tTamanho------------: %ld bytes\n", m->size);
        printf("\tData de Modificacao: %s\n", date);
        printf("-/-/-/-/-/-/-/-/--/-/-/-/-/-/-/-/-/-/-/-/-/-/\n");
        m = m->nextInOrder;
    }
}
