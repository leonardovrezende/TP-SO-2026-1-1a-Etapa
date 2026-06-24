#ifndef PCB_H
#define PCB_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct PCB PCB;

PCB *leProcesso(FILE *f);
void liberaProcesso(PCB *pcb);
void inicializaMutexCond(PCB *pcb);

#endif