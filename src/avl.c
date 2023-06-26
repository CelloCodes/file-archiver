#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "avl.h"
#include "fila.h"

int memberLessThan ( memberData_t* m1, memberData_t* m2 )
{
    unsigned int i = 0;
    while ((m1->name[i] != '\0') && (m2->name[i] != '\0')) {
        if (m1->name[i] < m2->name[i])
            return 1;
        else if (m1->name[i] > m2->name[i])
            return 0;
        
        i++;
    }

    if (m2->name[i] == '\0')
        return 0;

    return 1;
}

int memberGreaterThan ( memberData_t* m1, memberData_t* m2 )
{
    unsigned int i = 0;
    while ((m1->name[i] != '\0') && (m2->name[i] != '\0')) {
        if (m1->name[i] > m2->name[i])
            return 1;
        else if (m1->name[i] < m2->name[i])
            return 0;
        
        i++;
    }

    if (m1->name[i] == '\0')
        return 0;

    return 1;
}

int memberEquals ( memberData_t* m1, memberData_t* m2 )
{
    unsigned int i = 0;
    for (; (m1->name[i] != '\0') && (m1->name[i] == m2->name[i]); i++);

    if (m1->name[i] != m2->name[i])
        return 0;

    return 1;
}

// Retorna o menor valor encontrado a partir do nodo apontado por r
treeNode_t* nodeMin ( treeNode_t* r ) {
    while (r->leftSon)
        r = r->leftSon;
    return r;
}

// Retorna o maior valor encontrado a partir do nodo apontado por r
treeNode_t* nodeMax ( treeNode_t* r ) {
    while (r->rightSon)
        r = r->rightSon;
    return r;
}

// Executa uma rotacao direita direita e ajusta o balance dos nodos
void rRR ( treeNode_t** root, treeNode_t* p ) {
    treeNode_t* q = p->rightSon;
    treeNode_t* r = q->leftSon;

    if (! p->father)
        *root = r;
    else
        if (p == p->father->leftSon)
            p->father->leftSon = r;
        else
            p->father->rightSon = r;

    r->father = p->father;

    q->leftSon = r->rightSon;
    p->rightSon = r->leftSon;

    r->rightSon = q;
    r->leftSon = p;

    q->father = r;
    p->father = r;

    // Atualiza o balance de p e q baseado no balance de r
    // Caso o balance de r seja 0, significa que a rdd foi causada por
    // uma remocao na subarvore esquerda de p
    // Caso contrario, e necessario ver qual subarvore de r e a maior para
    // balancear p e q
    if (r->balance == 0) {
        if (p->rightSon)
            p->rightSon->father = p;
        if (q->leftSon)
            q->leftSon->father = q;

        p->balance = 0;
        q->balance = 0;        

    } else if (r->balance == 1) {
        if (p->rightSon)
            p->rightSon->father = p;
        q->leftSon->father = q;

        p->balance = -1;
        q->balance = 0;

    } else {
        if (q->leftSon)
            q->leftSon->father = q;
        p->rightSon->father = q;

        p->balance = 0;
        q->balance = 1;
    }

    r->balance = 0;
}

// Executa uma rotacao direita esquerda e ajusta o balance dos nodos
void rRL ( treeNode_t** root, treeNode_t* p ) {
    treeNode_t* q = p->rightSon;

    if (! p->father)
        *root = q;
    else
        if (p == p->father->leftSon)
            p->father->leftSon = q;
        else
            p->father->rightSon = q;
    
    q->father = p->father;
    p->rightSon = q->leftSon;
    if (q->leftSon)
        q->leftSon->father = p;
    p->father = q;
    q->leftSon = p;
    
    if (q->balance == 0) {
        q->balance = -1;
        p->balance = 1;
        
    } else {
        q->balance = 0;
        p->balance = 0;
    }
}

// Executa uma rotacao esquerda esquerda e ajusta o balance dos nodos
void rLL ( treeNode_t** root, treeNode_t* p ) {
    treeNode_t* q = p->leftSon;
    treeNode_t* r = q->rightSon;

    if (! p->father)
        *root = r;
    else
        if (p == p->father->leftSon)
            p->father->leftSon = r;
        else
            p->father->rightSon = r;

    r->father = p->father;

    q->rightSon = r->leftSon;
    p->leftSon = r->rightSon;

    r->leftSon = q;
    r->rightSon = p;

    q->father = r;
    p->father = r;

    // Atualiza o balance de p e q baseado no balance de r
    // Caso o balance de r seja 0, significa que a ree foi causada por
    // uma remocao na subarvore direita de p
    // Caso contrario, e necessario ver qual subarvore de r e a maior para
    // balancear p e q
    if (r->balance == 0) {
        if (q->rightSon)
            q->rightSon->father = q;
        if (p->leftSon)
            p->leftSon->father = p;

        p->balance = 0;
        q->balance = 0;

    } else if (r->balance == 1) {
        if (q->rightSon)
            q->rightSon->father = q;
        p->leftSon->father = p;

        p->balance = 0;
        q->balance = -1;

    } else {
        if (p->leftSon)
            p->leftSon->father = p;
        q->rightSon->father = q;

        p->balance = 1;
        q->balance = 0;
    }

    r->balance = 0;
}

// Executa uma rotacao esquerda direita e ajusta o balance dos nodos
void rLR ( treeNode_t** root, treeNode_t* p ) {
    treeNode_t* q = p->leftSon;

    if (! p->father)
        *root = q;
    else
        if (p == p->father->leftSon)
            p->father->leftSon = q;
        else
            p->father->rightSon = q;

    q->father = p->father;
    p->leftSon = q->rightSon;
    if (q->rightSon)
        q->rightSon->father = p;
    p->father = q;
    q->rightSon = p;

    if (q->balance == 0) {
        q->balance = 1;
        p->balance = -1;

    } else {
        q->balance = 0;
        p->balance = 0;
    }
}

void updateBalanceInsertion ( treeNode_t** root, treeNode_t* n ) {
    treeNode_t* p = n->father;

    if (n == p->leftSon)
        p->balance--;
    else
        p->balance++;

    treeNode_t *q = NULL;
    while ((p != *root) && (p->balance != 2) && (p->balance != -2)) {
        q = p;
        p = p->father;

        if (q->balance == 0)
            return;

        if (q == p->leftSon)
            p->balance--;
        else
            p->balance++;
    }

    if (p->balance == 2) {
        if (p->rightSon->balance == 1){
            rRL(root, p);
        } else {
            rRR(root, p);
        }

    } else if(p->balance == -2) {
        if (p->leftSon->balance == -1) {
            rLR(root, p);
        } else {
            rLL(root, p);
        }
    }
}

treeNode_t* treeInsert ( treeNode_t** root, memberData_t* key ) {
    treeNode_t* n = malloc(sizeof(treeNode_t));
    if (! n)
        return NULL;

    treeNode_t* atual = *root;
    treeNode_t* father = NULL;
    while (atual) {
        father = atual;

        if (memberLessThan(key, atual->key)) {
            atual = atual->leftSon;
        } else if (memberGreaterThan(key, atual->key)) {
            atual = atual->rightSon;
        } else {
            free(n);
            return NULL;
        }
    }

    n->key = key;
    n->balance = 0;
    n->father = father;
    n->rightSon = NULL;
    n->leftSon = NULL;
    if (! father) {
        *root = n;
    } else {
        if (memberEquals(key, father->key)) {
            free(n);
            return NULL;

        } else {
            if (memberLessThan(n->key, father->key)) {
                father->leftSon = n;
            } else {
                father->rightSon = n;
            }
        }
        updateBalanceInsertion(root, n);
    }

    return n;
}

void updateBalanceRemove ( treeNode_t** root, treeNode_t* p ) {
    if (! p)
        return;

    treeNode_t* q = NULL;
    while ((p != *root) && (p->balance != 2) && (p->balance != -2)) {
        q = p;
        p = p->father;

        if (q->balance != 0)
            return;

        if (q == p->leftSon)
            p->balance++;
        else
            p->balance--;
    }

    // Se o balance de p for 2, sera necessario eleftSontuar uma rde ou rdd
    // para aumentar a height do lado esquerdo e diminuir a do lado direito
    // Caso contrario sera necessario eleftSontuar uma red ou ree
    if (p->balance == 2) {

        // Se o balance de p eh 2, eh preciso saber qual o balance do seu
        // filho direito q para saber qual rotacao eleftSontuar
        // Se o balance de q for 1 ou 0, uma rde deve ser executada
        // Caso contrario, uma rdd deve ser exectuada
        if ((p->rightSon->balance == 1) || (p->rightSon->balance == 0)) {
            rRL(root, p);
        } else {
            rRR(root, p);
        }

    } else if (p->balance == -2) {

        // Se o balance de p eh -2, eh preciso saber qual o balance do seu
        // filho esquerdo q para saber qual rotacao eleftSontuar
        // Se o balance de q for -1 ou 0, uma red deve ser executada
        // Caso contrario, uma ree deve ser exectuada
        if ((p->leftSon->balance == -1) || (p->leftSon->balance == 0)) {
            rLR(root, p);
        } else {
            rLL(root, p);
        }
    }
}

void transplant ( treeNode_t** root, treeNode_t* u, treeNode_t* v) {
    if (! u->father)
        *root = v;
    else
        if (u == u->father->leftSon)
            u->father->leftSon = v;
        else
            u->father->rightSon = v;

    if (v)
        v->father = u->father;
}

int isLeaf ( treeNode_t* n ) {
    return ((! n->leftSon) && (! n->rightSon));
}

int treeRemove ( treeNode_t** root, memberData_t* key ) {
    treeNode_t* z = treeSearch(*root, key);
    if (! z)
        return 0;

    treeNode_t* nodoBalanco = z->father;

    // Primeira possibilidade
    // se o nodo for isLeaf, excluir e balancear
    // o balance ocorre a partir do father do nodo removido
    if (isLeaf(z)) {
        if (z == *root) {
            *root = NULL;
        } else {
            if (z == z->father->leftSon) {
                z->father->leftSon = NULL;
                z->father->balance++;
            } else {
                z->father->rightSon = NULL;
                z->father->balance--;
            }
        }

    // Segunda possibilidade
    // se o nodo tiver apenas um filho, colocar o filho no lugar e balancear
    // o balance ocorre a partir do father do nodo removido
    } else if (! z->leftSon) {
        transplant(root, z, z->rightSon);

        if (z->rightSon != *root) {
            if (z->rightSon == z->rightSon->father->leftSon) {
                z->rightSon->father->balance++;
            } else { 
                z->rightSon->father->balance--;
            }
        }

    } else if (! z->rightSon) {
        transplant(root, z, z->leftSon);

        if (z->leftSon != *root) {
            if (z->leftSon == z->leftSon->father->leftSon) {
                z->leftSon->father->balance++;
            } else {
                z->leftSon->father->balance--;
            }
        }

    // Terceira possibilidade
    // como o nodo tem dois filhos, buscar seu antecessor
    // se o antecessor tiver filho esquerdo, transplant para o seu lugar
    // colocar o antecessor no lugar do nodo a ser removidi e balancear
    // o balance ocorre a partir do father do antecessor caso ele nao seja filho
    // esquero do nodo removido, caso contrario o balance ocorre a partir do
    // antecessor
    } else {
        treeNode_t* y = nodeMax(z->leftSon);
        if (y != z->leftSon) {
            y->father->balance--;
            nodoBalanco = y->father;

            transplant(root, y, y->leftSon);
            y->leftSon = z->leftSon;
            y->leftSon->father = y;
            y->balance = z->balance;

        } else {
            nodoBalanco = y;
            y->balance = z->balance + 1;
        }

        transplant(root, z, y);
        y->rightSon = z->rightSon;
        y->rightSon->father = y;

        if (z == *root)
            *root = y;
    }

    updateBalanceRemove(root, nodoBalanco);

    free(z);

    return 1;
}

treeNode_t* treeSearch ( treeNode_t* nodo, memberData_t* key ) {
    if ((! nodo) || (memberEquals(nodo->key, key)))
        return nodo;

    if (memberLessThan(key, nodo->key))
        return treeSearch(nodo->leftSon, key);
    return treeSearch(nodo->rightSon, key);
}

// Calcula e retorna a height do nodo apontado por n
size_t height ( treeNode_t* n )
{
    size_t i = 0;
    while (n->father) {
        n = n->father;
        i++;
    }

    return i;
}

int treeWriteBFS ( FILE* dest, treeNode_t* root, long numNodes, int start ) {
    if (fseek(dest, start, SEEK_SET) == -1)
        return 2;
    
    if (fwrite(&numNodes, sizeof(numNodes), 1, dest) != 1)
        return 2;

    if (! root)
        return 0;

    struct fila *f = criaFila();
    if (! f)
        return 1;

    if (enfileirar(f, root) == 0) {
        destroiFila(f);
        return 1;
    }

    treeNode_t *n;
    memberData_t* m;
    while (filaVazia(f) == 0) {
        n = desenfileirar(f); 
        if (n){
            m = n->key;
            if (fwrite(&(m->sizeofName), sizeof(m->sizeofName), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(m->name, m->sizeofName, 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(&(m->permission), sizeof(m->permission), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(&(m->order), sizeof(m->order), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(&(m->position), sizeof(m->position), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(&(m->size), sizeof(m->size), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(&(m->UID), sizeof(m->UID), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if (fwrite(&(m->modDate), sizeof(m->modDate), 1, dest) != 1) {
                destroiFila(f);
                return 2;
            }

            if ((enfileirar(f, n->leftSon) == 0) || (enfileirar(f, n->rightSon) == 0)) {
                destroiFila(f);
                return 1;
            }
        }
    }

    destroiFila(f);

    return 0;
}

treeNode_t* freeTree ( treeNode_t* n )
{
    if (n) {
        freeTree(n->leftSon);
        freeTree(n->rightSon);
        free(n->key->name);
        free(n->key);
        free(n);
    }

    return NULL;
}
