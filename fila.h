#ifndef FILA_H
#define FILA_H

#include "PCB.h"

typedef struct FilaProntos FilaProntos;

FilaProntos *criaFila(int capacidade);
void liberaFila(FilaProntos *fila);

// Para FCFS e RR
void insereFila(FilaProntos *fila, PCB *pcb);  

// Para Fila de Prioridade
void insereHeap(FilaProntos *fila, PCB *pcb); 

// Remove do inicio (FCFS e RR)
PCB *removeFila(FilaProntos *fila);

// Remove um pcb específico (FCFS e RR)
PCB *removePcbFila(FilaProntos *fila, PCB *pcb);

// Remove da Fila de Prioridade
PCB *removeHeap(FilaProntos *fila);

// Verifica se a fila está vazia, com sicronização
int filaVazia(FilaProntos *fila);

// Verifica o tamanho da fila, com sicronização
int tamanhoFila(FilaProntos *fila);

// Marca generator_done como 1 e sinaliza condition variable da fila
void marcaGeradorPronto(FilaProntos *fila);

// Retorna 1 se o gerador está pronto, com sicronização
int geradorPronto(FilaProntos *fila);

// Espera surgir um novo processo na fila de prontos e retorna seu PCB (para FCFS e RR)
PCB *esperaProximo(FilaProntos *fila);

// Espera surgir um novo processo na fila de prontos e retorna seu PCB (para Fila de Prioridade)
PCB *esperaPrioritario(FilaProntos *fila);

// Retorna o PCB de maior prioridade da fila se sua prioridade for maior que a prioridade atual
PCB *maiorPrioridade(FilaProntos *fila, PCB *atual);

/* Mutex já usado internamente pela fila, exposto para permitir que os
 * escalonadores coordenem entre si (ex.: consultar o processo em execução
 * no outro processador) sem criar um novo lock. */
void travaFila(FilaProntos *fila);
void destravaFila(FilaProntos *fila);


#endif
