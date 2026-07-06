#ifndef PCB_H
#define PCB_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct PCB PCB;
typedef struct TCB TCB;  

typedef enum
{
   READY,
   RUNNING,
   FINISHED
} ProcessState;

PCB *leProcesso(FILE *f, int i);
void liberaProcesso(PCB *pcb);

// Inicia thread do processo além de criar um TCB para ela
TCB *criaThread(PCB *pcb, int tcbId);
int getNumThreads(PCB *pcb);
int getStartTime(PCB *pcb);

int getPid(PCB *pcb);
void setPid(PCB *pcb, int pid);
int getPriority(PCB *pcb);
int getRemainingTime(PCB *pcb);
void setRemainingTime(PCB *pcb, int t);

ProcessState getEstado(PCB *pcb);
void setEstado(PCB *pcb, ProcessState estado);

void travaPcb(PCB *pcb);
void destravaPcb(PCB *pcb);

// Função de wait da condition variable
void esperaPcb(PCB *pcb);

// Função de broadcast da condition variable
void sinalizaPcb(PCB *pcb);

// Espera todas as threads do processo encerrarem a execução
void joinThreads(PCB *pcb);

// Função de comparação do qsort, com intuito de ordenar os PCB's por ordem de chegada
int ordenaPCB(const void *arg1, const void *arg2);

#endif
