#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "PCB.h"

static void novaThread(TCB **threads, int n_threads, int aloc_threads, PCB *pcb, int pcbId){
    if(n_threads >= aloc_threads){
        aloc_threads *= 2;
        threads = realloc(threads, aloc_threads * sizeof(TCB*));  
    }
    
    threads[n_threads++] = criaThread(pcb, pcbId);
}

int main(int argc, char *argv[])
{
    FILE *f = fopen(argv[1], "r");

    int n_processos;
    int n_threads = 0;
    int aloc_threads = 3;
    TCB **threads = calloc(aloc_threads, sizeof(TCB*));

    fscanf(f, "%d%*c", &n_processos);
    PCB **pcb_list = calloc(n_processos, sizeof(PCB*));
    for (int i = 0; i < n_processos; i++){
        pcb_list[i] = leProcesso(f);
        for(int j=0; j < getNumThreads(pcb_list[i]); j++){
            novaThread(threads, n_threads, aloc_threads, pcb_list[i], j);
        }
    }

    //insere processos na fila por tempo de chegada
    PCB **filaProntos = calloc(n_processos, sizeof(PCB*));
    struct timeval inicio, agora;
    gettimeofday(&inicio, NULL);
    for(int i=0; i < n_processos; i++){
        usleep(getStartTime(pcb_list[i]));
        filaProntos[i] = pcb_list[i];
    }

    fclose(f);

    for (int i = 0; i < n_processos; i++)
        liberaProcesso(pcb_list[i]);
    free(pcb_list);
}