#ifndef ESCALONADOR_H
#define ESCALONADOR_H

#include "PCB.h"
#include "fila.h"

typedef enum
{
   POL_FCFS = 1,
   POL_RR = 2,
   POL_PRIORITY = 3
} TipoEscalonador;

#define QUANTUM_PADRAO_MS 500

typedef struct Escalonador Escalonador;

Escalonador *criaEscalonador(TipoEscalonador tipo, FilaProntos *fila, int quantum_ms);
void liberaEscalonador(Escalonador *esc);

void *rotinaEscalonador(void *arg);

#endif
