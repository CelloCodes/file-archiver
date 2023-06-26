#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "archiver.h"

#define BUFFER_SIZE 1024
#define HALF_BUFFER BUFFER_SIZE / 2

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

memberData_t* allocateMember ( char* name )
{
    unsigned short sizeofName = 0;
    for (; name[sizeofName] != '\0'; sizeofName++);
    sizeofName++;

    if ((sizeofName < 1) || (sizeofName >= 253))
        return NULL;

    memberData_t* m = malloc(sizeof(memberData_t));
    if (! m)
        return NULL;

    short nameDiff = 2;
    for (short i = 0; (i < sizeofName) && ((name[i] == '.') || (name[i] == '/')); i++)
        nameDiff--;
    
    m->name = malloc(sizeof(char) * (sizeofName + nameDiff));
    if (! m->name) {
        free(m);
        return NULL;
    }

    m->name[0] = '.';
    m->name[1] = '/';
    for (unsigned short i = 2; i < sizeofName + nameDiff; i++)
        m->name[i] = name[i - nameDiff];

    m->sizeofName = sizeofName + nameDiff;
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

    if (    (fread(m->name, sizeof(char) * m->sizeofName, 1, src) != 1)     ||
            (fread(&(m->permission), sizeof(m->permission), 1, src) != 1)   ||
            (fread(&(m->order), sizeof(m->order), 1, src) != 1)             ||
            (fread(&(m->position), sizeof(m->position), 1, src) != 1)       ||
            (fread(&(m->size), sizeof(m->size), 1, src) != 1)               ||
            (fread(&(m->UID), sizeof(m->UID), 1, src) != 1)                 ||
            (fread(&(m->modDate), sizeof(m->modDate), 1, src) != 1)         )
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
    if (    (fread(&posArchive, sizeof(posArchive), 1, src) != 1)           ||
            (fseek(src, posArchive, SEEK_SET) == -1)                        ||
            (fread(&(a->numMembers), sizeof(a->numMembers), 1, src) != 1)   )
        return 1;

    memberData_t* member;
    for (long i = 0; i < a->numMembers; i++) {
        member = malloc(sizeof(memberData_t));
        if (! member)
            return 2;

        member->name = NULL;
        member->nextInOrder = NULL;
        member->previousInOrder = NULL;

        if (! readMemberData(src, member)) {
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
int pasteBinary (FILE* src, FILE* dest, long srcStart, long destStart,
                    unsigned long bytes )
{
    if (fseek(dest, destStart, SEEK_SET) == -1)
        return 0;

    if (fseek(src, srcStart, SEEK_SET) == -1)
        return 0;

    void* buffer = malloc(BUFFER_SIZE);
    if (! buffer)
        return 0;

    while (bytes >= BUFFER_SIZE) {
        if (    (fread(buffer, BUFFER_SIZE, 1, src) != 1)   ||
                (fwrite(buffer, BUFFER_SIZE, 1, dest) != 1) ){

            free(buffer);
            return 0;
        }

        bytes -= BUFFER_SIZE;
    }

    if (bytes > 0) {
        if (    (fread(buffer, bytes, 1, src) != 1)     ||
                (fwrite(buffer, bytes, 1, dest) != 1)   ){

            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;
}

int adjustBinaryRight ( FILE* dest,  long bufSize, long start, long stop,
                        unsigned long bytes )
{
    if (fseek(dest, start, SEEK_SET) == -1)
        return 0;

    void* buffer = malloc(bufSize);
    if (! buffer)
        return 0;

    long curPos = ftell(dest);
    if (curPos < 0) {
        free(buffer);
        return 0;
    }

    while ((curPos - bufSize) >= stop) {
        if (fseek(dest, curPos-bufSize, SEEK_SET) == -1) {
            free(buffer);
            return 0;
        }

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buffer);
            return 0;
        }

        if (    (fread(buffer, bufSize, 1, dest) != 1)  || 
                (fseek(dest, curPos+bytes, SEEK_SET) == -1) ||
                (fwrite(buffer, bufSize, 1, dest) != 1) ||
                (fseek(dest, curPos, SEEK_SET) == -1)       ){

            free(buffer);
            return 0;
        }
    }

    if (curPos > stop) {
        long toRead = curPos - stop;
        if (    (fseek(dest, stop, SEEK_SET) == -1)         ||
                (fread(buffer, toRead, 1, dest) != 1)       ||
                (fseek(dest, stop+bytes, SEEK_SET) == -1)   ||
                (fwrite(buffer, toRead, 1, dest) != 1)      ){

            free(buffer);
            return 0;
        }
    }
    
    free(buffer);

    return 1;
}

int adjustBinaryLeft (  FILE* dest, long bufSize, long start, long stop,
                        unsigned long bytes )
{
    if (fseek(dest, start, SEEK_SET) == -1)
        return 0;

    long curPos = ftell(dest);
    if (curPos < 0)
        return 0;

    void* buffer = malloc(bufSize);
    if (! buffer)
        return 0;

    while ((curPos + bufSize) < stop) {
        if (    (fread(buffer, bufSize, 1, dest) != 1)          || 
                (fseek(dest, curPos + bytes, SEEK_SET) == -1)   ||
                (fwrite(buffer, bufSize, 1, dest) != 1)         ||
                (fseek(dest, -bytes, SEEK_CUR) == -1)           ){
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
        if (    (fread(buffer, toRead, 1, dest) != 1)           ||
                (fseek(dest, curPos + bytes, SEEK_SET) == -1)   ||
                (fwrite(buffer, toRead, 1, dest) != 1)          ){

            free(buffer);
            return 0;
        }
    }

    
    free(buffer);

    return 1;
}

void adjustPositions ( memberData_t* start, memberData_t* stop, long sizeDiff )
{
    while (start != stop) {
        start->position += sizeDiff;
        start = start->nextInOrder;
    }
}

int insertMember ( FILE* src, FILE* dest, char* srcName, archive_t* a )
{
    memberData_t* member = allocateMember(srcName);
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

        if (sizeDiff > 0) {
            if (! adjustBinaryRight(dest, BUFFER_SIZE,
                        oldMember->position + oldMember->size,
                        getFirstFreeByte(a), sizeDiff)) {
                freeMember(member);
                return 5;
            } else {
                adjustPositions(oldMember->nextInOrder, NULL, sizeDiff) ;
            }
        } else if (sizeDiff < 0) {
            if (! adjustBinaryLeft(dest, BUFFER_SIZE,
                        oldMember->position + oldMember->size,
                        getFirstFreeByte(a),
                        sizeDiff)) {
                freeMember(member);
                return 5;
            } else {
                adjustPositions(oldMember->nextInOrder, NULL, sizeDiff) ;
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
        member->position = pastePosition;
        insertInOrder(a, member);
        a->numMembers++;
    }

    if (! pasteBinary(src, dest, 0, pastePosition, s.st_size))
        return 5;

    return 0;
}

int updateMember ( FILE* src, FILE* dest, char* srcName, archive_t* a )
{
    memberData_t* member = allocateMember(srcName);
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
            if (! adjustBinaryRight(dest, BUFFER_SIZE,
                        oldMember->position + oldMember->size,
                        getFirstFreeByte(a),
                        sizeDiff)) {
                freeMember(member);
                return 5;
            } else {
                adjustPositions(oldMember->nextInOrder, NULL, sizeDiff) ;
            }
        } else if (sizeDiff < 0) {
            if (! adjustBinaryLeft(dest, BUFFER_SIZE,
                        oldMember->position + oldMember->size, 
                        getFirstFreeByte(a),
                        sizeDiff)) {
                freeMember(member);
                return 5;
            } else {
                adjustPositions(oldMember->nextInOrder, NULL, sizeDiff) ;
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

    if (! pasteBinary(src, dest, 0, pastePosition, s.st_size))
        return 5;

    return 0;
}

// moves size bytes "rotating" the memory region right to left
int circularMoveRightToLeft ( FILE* dest, long start, long stop, long size )
{
    void* buf = malloc(HALF_BUFFER);
    if (! buf)
        return 0;

    long curPos;
    long tamRead = 0;
    while (tamRead + HALF_BUFFER <= size) {
        if (fseek(dest, start, SEEK_SET) == -1)
            return 0;

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buf);
            return 0;
        }

        if ((fread(buf, HALF_BUFFER, 1, dest) != 1)                             ||
            (! adjustBinaryLeft(dest, HALF_BUFFER, start+HALF_BUFFER,
                                stop, -HALF_BUFFER)) ||
            (fwrite(buf, HALF_BUFFER, 1, dest) != 1)                            ){

            free(buf);
            return 0;
        }

        tamRead += HALF_BUFFER;
    }

    if (tamRead < size) {
        if (fseek(dest, start, SEEK_SET) == -1)
            return 0;

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buf);
            return 0;
        }

        if ((fread(buf, size - tamRead, 1, dest) != 1)                          ||
            (! adjustBinaryLeft(dest, HALF_BUFFER, curPos, stop, size-tamRead)) ){

            free(buf);
            return 0;
        }

        tamRead += size - tamRead;
    }

    free(buf);

    return 1;
}

int circularMoveLeftToRight ( FILE* dest, long start, long stop, long size )
{
    void* buf = malloc(HALF_BUFFER);
    if (! buf)
        return 0;

    long curPos;
    long tamRead = 0;
    while (tamRead + HALF_BUFFER <= size) {
        if (fseek(dest, stop - HALF_BUFFER, SEEK_SET) == -1)
            return 0;

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buf);
            return 0;
        }

        if ((fread(buf, HALF_BUFFER, 1, dest) != 1)                 ||
            (! adjustBinaryRight(dest, HALF_BUFFER,
                                    start, stop, -HALF_BUFFER))     ||
            (fseek(dest, start, SEEK_SET) == -1)                    ||
            (fwrite(buf, HALF_BUFFER, 1, dest) != 1)                ){

            free(buf);
            return 0;
        }

        tamRead += HALF_BUFFER;
    }

    if (tamRead < size) {
        if (fseek(dest, start, SEEK_SET) == -1)
            return 0;

        curPos = ftell(dest);
        if (curPos < 0) {
            free(buf);
            return 0;
        }

        if ((fread(buf, size - tamRead, 1, dest) != 1)                          ||
            (! adjustBinaryLeft(dest, HALF_BUFFER, curPos, stop, size-tamRead)) ){

            free(buf);
            return 0;
        }

        tamRead += size - tamRead;
    }

    free(buf);

    return 1;
}


memberData_t* getMember ( archive_t* a, char* name )
{
    memberData_t* m = allocateMember(name);
    if (! m)
        return NULL;

    treeNode_t* tn = treeSearch(a->memberTree, m);
    freeMember(m);

    if (! tn)
        return NULL;
    return tn->key;
}

int moveMember ( FILE* dest, archive_t* a, char* m1Name, char* m2Name ) 
{
    memberData_t* m1 = getMember(a, m1Name);
    memberData_t* m2 = getMember(a, m2Name);
    memberData_t* mAux = NULL;
    if ((! m1) || (! m2))
        return 1;

    if (m1->name == m2->name)
        return 1;

    if (m1->order < m2->order) {
        if (! circularMoveRightToLeft(dest, m1->position,
                                        m2->position+m2->size, m1->size))
            return 1;

        adjustPositions(m1->nextInOrder, m2->nextInOrder, -m1->size);
        for(mAux = m1->nextInOrder; mAux != m2->nextInOrder; mAux = mAux->nextInOrder)
            mAux->order--;
        m1->order = m2->order + 1;
        m1->position = m2->position + m2->size;

        if (m1->previousInOrder)
            m1->previousInOrder->nextInOrder = m1->nextInOrder;
        else
            a->firstInOrder = m1->nextInOrder;

        m1->nextInOrder->previousInOrder = m1->previousInOrder;

        m1->nextInOrder = m2->nextInOrder;
        if (m1->nextInOrder)
            m1->nextInOrder->previousInOrder = m1;
        else
            a->lastInOrder = m1;

        m1->previousInOrder = m2;
        m2->nextInOrder= m1;

    } else {
        if (m1->order == (m2->order + 1))
            return 0;

        if (! circularMoveLeftToRight(dest, m2->position + m2->size,
                                        m1->position + m1->size, m1->size))
            return 1;

        adjustPositions(m2->nextInOrder, m1, m1->size);
        for(mAux = m2->nextInOrder; mAux != m1; mAux = mAux->nextInOrder)
            mAux->order++;
        m1->order = m2->order + 1;
        m1->position = m2->position + m2->size;

        m1->previousInOrder->nextInOrder = m1->nextInOrder;

        if (m1->nextInOrder)
            m1->nextInOrder->previousInOrder = m1->previousInOrder;
        else
            a->lastInOrder = m1->previousInOrder;

        m1->nextInOrder = m2->nextInOrder;
        m1->nextInOrder->previousInOrder = m1;

        m1->previousInOrder = m2;
        m2->nextInOrder= m1;
    }

    return 0;
}

int removeMember ( FILE* dest, archive_t* a, char* m1Name ) 
{
    memberData_t* m1 = getMember(a, m1Name);
    memberData_t* m2 = a->lastInOrder;
    memberData_t* mAux = NULL;
    if ((! m1) || (! m2))
        return 1;

    if (m1->name == m2->name) {
        a->lastInOrder = m1->previousInOrder;    
        if (m1->previousInOrder)
            m1->previousInOrder->nextInOrder = NULL;
        else
            a->firstInOrder = NULL;
        
        if (! treeRemove(&(a->memberTree), m1))
            return 1;

        freeMember(m1);
        a->numMembers--;

        return 0;
    }

    if (! circularMoveRightToLeft(dest, m1->position,
                                    m2->position+m2->size, m1->size))
        return 1;

    adjustPositions(m1->nextInOrder, m2->nextInOrder, -m1->size);
    for(mAux = m1->nextInOrder; mAux != m2->nextInOrder; mAux = mAux->nextInOrder)
        mAux->order--;

    if (m1->previousInOrder)
        m1->previousInOrder->nextInOrder = m1->nextInOrder;
    else
        a->firstInOrder = m1->nextInOrder;

    m1->nextInOrder->previousInOrder = m1->previousInOrder;

    if (! treeRemove(&(a->memberTree), m1))
        return 1;

    freeMember(m1);
    a->numMembers--;

    return 0;
}

int createDirectoryHierarchy ( short strLen, char* path )
{
    char dirs[strLen];

    short lastEnd = 0;
    short j = 0;
    for (int i = 2; i < strLen; i++) {
        if(path[i] == '/') {
            dirs[i+1] = '\0';
            for(j = i; j >= lastEnd; j--){
                dirs[j] = path[j];
            }
            if (mkdir(dirs, S_IRWXU)) {
                if (errno != EEXIST) {
                    fprintf(stderr, "Falha ao criar arvore de diretorios %s\n", path);
                    return 0;
                }
            }
            lastEnd = i;
        }
    }

    return 1;
}

int extractAllMembers ( FILE* src, archive_t* a )
{
    memberData_t* m = a->firstInOrder;
    if (! m)
        return 0;

    int error;
    FILE* dest;
    while (m) {
        if (! createDirectoryHierarchy (m->sizeofName, m->name))
            return 1;

        dest = fopen(m->name, "w");
        if (! dest) {
            fprintf(stderr, "Falhou ao criar arquivo %s\n", m->name);
            return 1;
        }

        error = extractMember(src, dest, m->name, a);
        if (error) {
            fclose(dest);
            fprintf(stderr, "Falhou ao extrair arquivo %s\n", m->name);
            return 1;
        }

        fclose(dest);
        dest = NULL;
        m = m->nextInOrder;
    }

    return 0;
}

int extractMember ( FILE* src, FILE* dest, char* name, archive_t* a )
{
    memberData_t* m1 = getMember(a, name);
    if (! m1) {
        printf("nao encontrou membro\n");
        return 1;
    }

    if (! pasteBinary(src, dest, m1->position, 0, m1->size)) {
        printf("nao colou binario no destino\n");
        return 1;
    }

    return 0;
}

int writeArchive ( FILE* dest, archive_t* a )
{
    if (fseek(dest, 0, SEEK_SET) == -1)
        return 1;

    int pos = getFirstFreeByte(a);
    if (fwrite(&pos, sizeof(int), 1, dest) != 1)
        return 1;

    return treeWriteBFS(dest, a->memberTree, a->numMembers, pos);
}

void printArchive ( archive_t* a )
{
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
        strftime(date, sizeof(date), "%Y-%m-%d %H:%M", &ts);
            
        printf("%s %d %8ld %s %s\n", perm, m->UID, m->size, date, m->name);
        m = m->nextInOrder;
    }
}
