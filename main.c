#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include "PCB.h"
#include "TCB.h"
#include "fila.h"
#include "escalonador.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "uso: %s <arquivo>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("fopen");
        return 1;
    }

    int n_processos;
    if (fscanf(f, "%d", &n_processos) != 1) {
        fprintf(stderr, "arquivo invalido\n");
        fclose(f);
        return 1;
    }

    int n_threads = 0;
    int coloc_threads = 0;
    
    PCB **pcb_list = calloc(n_processos, sizeof(PCB*));
    for (int i = 0; i < n_processos; i++){
        pcb_list[i] = leProcesso(f, i + 1);
        n_threads += getNumThreads(pcb_list[i]);
    }
    int politica;
    if(fscanf(f, "%d", &politica) != 1){
        fprintf(stderr, "politica invalido\n");
        fclose(f);
        return 1;
    }

    fclose(f);

    qsort(pcb_list, n_processos, sizeof(PCB*), ordenaPCB);
    //insere processos na fila por tempo de chegada
    FilaProntos *fila = criaFila(n_processos);
    Escalonador *esc = criaEscalonador((TipoEscalonador)politica, fila, QUANTUM_PADRAO_MS);

    TCB **threads = calloc(n_threads, sizeof(TCB*));
    int tempoAnterior = 0;
    for(int i=0; i < n_processos; i++){
        usleep((getStartTime(pcb_list[i]) - tempoAnterior) * 1000);
        tempoAnterior = getStartTime(pcb_list[i]);
        for(int j=0; j < getNumThreads(pcb_list[i]); j++){
            threads[coloc_threads++] = criaThread(pcb_list[i], j);
        }
        insereFila(fila, pcb_list[i]);
    }

    pthread_t esc_tid;
    pthread_create(&esc_tid, NULL, rotinaEscalonador, esc);
    marcaGeradorPronto(fila);

    pthread_join(esc_tid, NULL);

    for(int i = 0; i < n_processos; i++)
        joinThreads(pcb_list[i]);

    for (int i = 0; i < n_processos; i++)
        liberaProcesso(pcb_list[i]);
    free(pcb_list);
    free(threads);
    liberaFila(fila);
    liberaEscalonador(esc);

    return 0;
}