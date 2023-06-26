#include <stdlib.h>

#include "fila.h"

struct fila *criaFila()
{
	struct fila *f = malloc(sizeof(struct fila));
	if (f != NULL) {
		f->ini = NULL;
		f->fim = NULL;
	}
	return f;
}

struct fila *destroiFila(struct fila *f)
{
	if (f != NULL) {
		struct nodo_fila *temp = NULL;
		while (f->ini != NULL) {
			temp = f->ini;
			f->ini = f->ini->prox;
			free(temp);
		}
		f->fim = NULL;
	}
	free(f);
	
	return NULL;
}

int filaVazia(struct fila *f)
{
	if (f != NULL)
		return (f->ini == NULL);
	return 0;
}

int enfileirar(struct fila *f, void *n)
{
	struct nodo_fila *nf = malloc(sizeof(struct nodo_fila));
	if ((f != NULL) && (nf != NULL)) {
		nf->n = n;
		nf->prox = NULL; 
		if (f->ini != NULL) {
			f->fim->prox = nf;
			f->fim = nf;
		} else {
			f->ini = nf;
			f->fim = nf;
		}

		return 1;
	}

	return 0;
}

void *desenfileirar(struct fila *f)
{
	void *n = NULL;
	if (f != NULL) {
		n = f->ini->n;
		struct nodo_fila *nodo = f->ini->prox;
		if (f->ini == f->fim)
			f->fim = NULL;
		free(f->ini);
		f->ini = nodo;
	}

	return n;
}
