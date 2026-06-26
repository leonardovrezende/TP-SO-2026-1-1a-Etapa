#ifndef PCB_H
#define PCB_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "TCB.h"

typedef struct PCB PCB;

PCB *leProcesso(FILE *f);
void liberaProcesso(PCB *pcb);
TCB *criaThread(PCB *pcb, int tcbId);
int getNumThreads(PCB *pcb);
int getStartTime(PCB *pcb);

#endif