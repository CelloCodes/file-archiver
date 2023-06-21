#ifndef AVL_H_
#define AVL_H_

typedef struct {
    // ideia: primeiro byte de cada nome ser o tamanho do nome
    // dessa forma le-se o tamanho do nome, aloca-se espaco para ele
    // e entao le-se o nome, cujo tamanho ja e conhecido
    // O tamanho+1 deve ser alocado devido ao caractere nulo ao final
    // Lembrar de escrever o byte de tamanho quando for salvar o nome
    char* name; 
    unsigned short sizeofName;

    unsigned short permission;

    unsigned int position;
    unsigned int order;
    unsigned int size;
    unsigned int UID;

    unsigned long modDate;

} memberData_t;


typedef struct treeNode_t {
    memberData_t* key;
    char balance;
    struct treeNode_t* father;
    struct treeNode_t* leftSon;
    struct treeNode_t* rightSon;
} treeNode_t;

//retorna NULL se não foi possível inserir
treeNode_t* treeInsert ( treeNode_t** raiz, memberData_t* chave );

//retorna 1 caso removido, ou 0 caso não seja possível remover
int treeRemove ( treeNode_t** raiz, memberData_t* chave );

//retorna NULL se não existe
treeNode_t* treeSearch ( treeNode_t* nodo, memberData_t* chave );

void treePrintDFS ( treeNode_t* nodo );

void treePrintBFS( treeNode_t* raiz );

//retorna NULL e desaloca a memoria da arvore
treeNode_t* freeTree ( treeNode_t* n );

#endif // AVL_H_
