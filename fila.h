#ifndef FILA_H
#define FILA_H

struct nodo_fila {
	void* n;
	struct nodo_fila* prox;
};

struct fila {
	struct nodo_fila* ini;
	struct nodo_fila* fim;
};

// Aloca uma fila dinamicamente
// Retorna o endereco da fila alocada em caso de sucesso
// Retorna NULL caso contrario
struct fila* criaFila();

// Desaloca a fila apontada por f e suas estruturas internas
// Retorna NULL
struct fila* destroiFila(struct fila* f);

// Retorna 1 se a fila apontada por f estiver vazia
// Retorna 0 caso contrario
int filaVazia(struct fila* f);

// Insere o ponteiro n no final da fila apontada por f
// Retorna 1 em caso de sucesso
// Retorna 0 caso contrario
int enfileirar(struct fila* f, void* n);

// Obtem o primeiro elemento da fila apontada por f
// Retorna o ponteiro armazenado na primeira posicao em caso de sucesso
// Retorna NULL caso contrario
void* desenfileirar(struct fila* f);

#endif
