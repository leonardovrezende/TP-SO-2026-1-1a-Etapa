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
void esperaPcb(PCB *pcb);
void sinalizaPcb(PCB *pcb);
void joinThreads(PCB *pcb);
int ordenaPCB(const void *arg1, const void *arg2);

#endif
