#include <stdio.h>
#include "PCB.h"

int main(int argc, char *argv[])
{
    FILE *f = fopen(argv[1], "r");

    int n_processos;

    fscanf(f, "%d%*c", &n_processos);
    PCB **pcb_list = calloc(n_processos, sizeof(PCB *));
    for (int i = 0; i < n_processos; i++)
        pcb_list[i] = leProcesso(f);

    fclose(f);

    for (int i = 0; i < n_processos; i++)
        liberaProcesso(pcb_list[i]);
    free(pcb_list);
}