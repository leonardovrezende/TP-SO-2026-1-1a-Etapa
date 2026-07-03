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
    TCB **threads = calloc(n_threads, sizeof(TCB*));

    Log *log = inicializaLog();
    pthread_t esc_tid, esc_tid2;
    Escalonador *esc = criaEscalonador((TipoEscalonador)politica, fila, QUANTUM_PADRAO_MS, 0, log);
    Escalonador *esc2;
    pthread_create(&esc_tid, NULL, rotinaEscalonador, esc);
    if(NUM_CPUS == 2){
        esc2 = criaEscalonador((TipoEscalonador)politica, fila, QUANTUM_PADRAO_MS, 1, log);
        pthread_create(&esc_tid2, NULL, rotinaEscalonador, esc2);
    }

    int tempoAnterior = 0;
    for(int i=0; i < n_processos; i++){
        usleep((getStartTime(pcb_list[i]) - tempoAnterior) * 1000);
        tempoAnterior = getStartTime(pcb_list[i]);
        for(int j=0; j < getNumThreads(pcb_list[i]); j++){
            threads[coloc_threads++] = criaThread(pcb_list[i], j);
        }
        if(politica == 3) insereHeap(fila, pcb_list[i]);
        else insereFila(fila, pcb_list[i]);
    }
    marcaGeradorPronto(fila);

    pthread_join(esc_tid, NULL);
    if(NUM_CPUS == 2) pthread_join(esc_tid2, NULL);

    for(int i = 0; i < n_processos; i++){
        joinThreads(pcb_list[i]);
        //logWrite(log, "[PRIORITY] Processo PID %d finalizado\n", getPid(pcb_list[i]));
    }

    logWrite(log, "Escalonador terminou execução de todos processos\n");
    liberaLog(log);

    for (int i = 0; i < n_processos; i++)
        liberaProcesso(pcb_list[i]);
    free(pcb_list);

    for (int i = 0; i < n_threads; i++)
        liberaTCB(threads[i]);
    free(threads);

    liberaFila(fila);
    liberaEscalonador(esc);
    if(NUM_CPUS == 2) liberaEscalonador(esc2);

    return 0;
}