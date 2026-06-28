#ifndef FILA_H
#define FILA_H

#include "PCB.h"

typedef struct FilaProntos FilaProntos;

FilaProntos *criaFila(int capacidade);
void liberaFila(FilaProntos *fila);

void insereFila(FilaProntos *fila, PCB *pcb);     
PCB *removeFila(FilaProntos *fila);               
PCB *removePcbFila(FilaProntos *fila, PCB *pcb);  

int filaVazia(FilaProntos *fila);
int tamanhoFila(FilaProntos *fila);

void marcaGeradorPronto(FilaProntos *fila);
int geradorPronto(FilaProntos *fila);

PCB *esperaProximo(FilaProntos *fila);

#endif
