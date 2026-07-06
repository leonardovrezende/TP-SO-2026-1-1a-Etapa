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
#ifndef NUM_CPUS
#define NUM_CPUS 1
#endif

typedef struct log Log;
typedef struct Escalonador Escalonador;

// Funções para escrita no arquivo de log_execucao_minikernel
Log *inicializaLog();
void liberaLog(Log *l);
void logWrite(Log *l, const char *fmt, ...);

Escalonador *criaEscalonador(TipoEscalonador tipo, FilaProntos *fila, int quantum_ms, int cpu_id, Log *log);
void liberaEscalonador(Escalonador *esc);

// Para o caso multiprocessador
void defineParEscalonador(Escalonador *esc, Escalonador *par);

// Função de rotina da thread escalonadora
void *rotinaEscalonador(void *arg);

#endif
