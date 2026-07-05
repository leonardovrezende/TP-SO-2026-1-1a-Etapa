#ifndef FILA_H
#define FILA_H

#include "PCB.h"

typedef struct FilaProntos FilaProntos;

FilaProntos *criaFila(int capacidade);
void liberaFila(FilaProntos *fila);

void insereFila(FilaProntos *fila, PCB *pcb);     
void insereHeap(FilaProntos *fila, PCB *pcb); 

PCB *removeFila(FilaProntos *fila);
PCB *removePcbFila(FilaProntos *fila, PCB *pcb);
PCB *removeHeap(FilaProntos *fila);

int filaVazia(FilaProntos *fila);
int tamanhoFila(FilaProntos *fila);

void marcaGeradorPronto(FilaProntos *fila);
int geradorPronto(FilaProntos *fila);

PCB *esperaProximo(FilaProntos *fila);

PCB *esperaPrioritario(FilaProntos *fila);
PCB *maiorPrioridade(FilaProntos *fila, PCB *atual);

/* Mutex já usado internamente pela fila, exposto para permitir que os
 * escalonadores coordenem entre si (ex.: consultar o processo em execução
 * no outro processador) sem criar um novo lock. */
void travaFila(FilaProntos *fila);
void destravaFila(FilaProntos *fila);


#endif
