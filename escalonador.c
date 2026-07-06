#include "escalonador.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

struct log {
   FILE *log_file;
   pthread_mutex_t mutex_file;
};

struct Escalonador
{
   TipoEscalonador tipo;
   FilaProntos *fila;
   int quantum_ms;
   int cpu_id;

   PCB *current_process;
   Log *log;

   struct Escalonador *par; /* outro processador, para ajudar processos multithread ociosos */
};

Log *inicializaLog(){
   Log *l = calloc(1, sizeof(Log));
   l->log_file = fopen("log_execucao_minikernel.txt", "w");
   pthread_mutex_init(&l->mutex_file, NULL);
   return l;
}

void liberaLog(Log *l){
   fclose(l->log_file);
   pthread_mutex_destroy(&l->mutex_file);
   free(l);
}

void logWrite(Log *l, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pthread_mutex_lock(&l->mutex_file);
    vfprintf(l->log_file, fmt, args);
    pthread_mutex_unlock(&l->mutex_file);
    va_end(args);
}

Escalonador *criaEscalonador(TipoEscalonador tipo, FilaProntos *fila, int quantum_ms, int cpu_id, Log *log)
{
   Escalonador *esc = calloc(1, sizeof(Escalonador));
   esc->tipo = tipo;
   esc->fila = fila;
   esc->quantum_ms = quantum_ms;
   esc->cpu_id = cpu_id;
   esc->current_process = NULL;
   esc->log = log;
   esc->par = NULL;
   return esc;
}

void liberaEscalonador(Escalonador *esc)
{
   free(esc);
}

void defineParEscalonador(Escalonador *esc, Escalonador *par)
{
   esc->par = par;
}

static void despacha(Escalonador *esc, PCB *pcb)
{
   travaPcb(pcb);
   /* a ultima thread do processo pode ter marcado FINISHED entre o
    * escalonador ler "nao terminou" e conseguir o lock aqui de novo;
    * nesse caso nao ha mais threads vivas para sinalizar de volta, e
    * sobrescrever para RUNNING travaria o escalonador para sempre. */
   if (getEstado(pcb) != FINISHED)
      setEstado(pcb, RUNNING);
   sinalizaPcb(pcb);
   destravaPcb(pcb);

   travaFila(esc->fila);
   esc->current_process = pcb;
   destravaFila(esc->fila);
}

/* Processador ocioso (fila vazia): se o outro processador está executando
 * um processo com mais de uma thread, ajuda executando "outra thread" dele
 * em vez de ficar parado, conforme a modelagem 1:1 de threads da spec. */
static int ajudaProcessoParalelo(Escalonador *esc)
{
   if (esc->par == NULL)
      return 0;

   travaFila(esc->fila);
   PCB *alvo = esc->par->current_process;
   destravaFila(esc->fila);

   if (alvo == NULL)
      return 0;

   travaPcb(alvo);
   if (getEstado(alvo) != RUNNING || getNumThreads(alvo) < 2) {
      destravaPcb(alvo);
      return 0;
   }

   if (esc->tipo == POL_PRIORITY)
      logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d // processador %d\n",
               getPid(alvo), getPriority(alvo), esc->cpu_id);
   else if (esc->tipo == POL_RR)
      logWrite(esc->log, "[RR] Executando processo PID %d com quantum %dms // processador %d\n",
               getPid(alvo), esc->quantum_ms, esc->cpu_id);
   else
      logWrite(esc->log, "[FCFS] Executando processo PID %d // processador %d\n",
               getPid(alvo), esc->cpu_id);

   /* O dono pode redespachar o mesmo PCB em pedaços (uma thread termina seu
    * quantum e volta a READY antes das outras); continuamos ajudando
    * enquanto o dono ainda for responsável por este processo. Mas paramos
    * assim que outro processo ficar pronto na fila: os processadores dão
    * vazão a PROCESSOS, e só atendem threads do mesmo processo em paralelo
    * quando não há mais de um processo pronto (spec, obs.txt). */
   int mesmoDono = 1;
   int filaTemProcesso = 0;
   while (getEstado(alvo) != FINISHED && mesmoDono && !filaTemProcesso) {
      while (getEstado(alvo) == RUNNING)
         esperaPcb(alvo);
      if (getEstado(alvo) == FINISHED)
         break;

      destravaPcb(alvo);
      usleep(1000);
      travaFila(esc->fila);
      mesmoDono = (esc->par->current_process == alvo);
      destravaFila(esc->fila);
      filaTemProcesso = !filaVazia(esc->fila);
      travaPcb(alvo);
   }
   destravaPcb(alvo);

   return 1;
}

static PCB *proximoProcesso(Escalonador *esc)
{
   int prioridade = (esc->tipo == POL_PRIORITY);

   if (esc->par == NULL)
      return prioridade ? esperaPrioritario(esc->fila) : esperaProximo(esc->fila);

   while (1) {
      PCB *pcb = prioridade ? removeHeap(esc->fila) : removeFila(esc->fila);
      if (pcb != NULL)
         return pcb;
      if (filaVazia(esc->fila) && geradorPronto(esc->fila))
         return NULL;
      if (!ajudaProcessoParalelo(esc))
         usleep(1000);
   }
}

static void escalonaFCFS(Escalonador *esc)
{
   PCB *pcb;
   while ((pcb = proximoProcesso(esc)) != NULL)
   {
      if(NUM_CPUS == 2) logWrite(esc->log, "[FCFS] Executando processo PID %d // processador %d\n", getPid(pcb), esc->cpu_id);
      else logWrite(esc->log, "[FCFS] Executando processo PID %d\n", getPid(pcb));

      int finalizado;
      do {
         despacha(esc, pcb);
         travaPcb(pcb);
         while (getEstado(pcb) == RUNNING)
            esperaPcb(pcb); /* aguarda threads sinalizarem READY ou FINISHED */
         finalizado = (getEstado(pcb) == FINISHED); /* lido ainda com o mutex do pcb travado */
         destravaPcb(pcb);
      } while (!finalizado);

      logWrite(esc->log, "[FCFS] Processo PID %d finalizado\n", getPid(pcb));
      travaFila(esc->fila);
      esc->current_process = NULL;
      destravaFila(esc->fila);
   }
}

static void escalonaRR(Escalonador *esc)
{
   PCB *pcb;
   while ((pcb = proximoProcesso(esc)) != NULL)
   {
      if(NUM_CPUS == 2) logWrite(esc->log, "[RR] Executando processo PID %d com quantum %dms // processador %d\n",
              getPid(pcb), esc->quantum_ms, esc->cpu_id);
      else logWrite(esc->log, "[RR] Executando processo PID %d com quantum %dms\n",
              getPid(pcb), esc->quantum_ms);

      despacha(esc, pcb);

      usleep(esc->quantum_ms * 1000);

      travaPcb(pcb);
      while (getEstado(pcb) == RUNNING)
         esperaPcb(pcb); /* aguarda thread sinalizar FINISHED ou READY */
      if (getEstado(pcb) == FINISHED) {
         destravaPcb(pcb);
         logWrite(esc->log, "[RR] Processo PID %d finalizado\n", getPid(pcb));
      } else {
         setEstado(pcb, READY);
         destravaPcb(pcb);
         insereFila(esc->fila, pcb);
      }
      travaFila(esc->fila);
      esc->current_process = NULL;
      destravaFila(esc->fila);
   }
}

static void escalonaPrioridade(Escalonador *esc)
{
   PCB *pcb;
   while ((pcb = proximoProcesso(esc)) != NULL)
   {
      if(NUM_CPUS == 2) logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d // processador %d\n",
              getPid(pcb), getPriority(pcb), esc->cpu_id);
      else logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d\n",
              getPid(pcb), getPriority(pcb));

      int finalizado;
      do
      {
         int preempted = 0;
         despacha(esc, pcb);

         travaPcb(pcb);
         while (getEstado(pcb) == RUNNING)
         {
            PCB *prox = maiorPrioridade(esc->fila, pcb);
            if (prox != NULL)
            {
               setEstado(pcb, READY);
               destravaPcb(pcb);
               insereHeap(esc->fila, pcb);
               pcb = prox;
               preempted = 1;

               if(NUM_CPUS == 2) logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d // processador %d\n",
                        getPid(pcb), getPriority(pcb), esc->cpu_id);
               else logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d\n",
                        getPid(pcb), getPriority(pcb));
               break;
            }
            esperaPcb(pcb);
         }

         if (preempted) {
            finalizado = 0;
            continue;
         }

         finalizado = (getEstado(pcb) == FINISHED); /* lido ainda com o mutex do pcb travado */
         destravaPcb(pcb);
      } while (!finalizado);

      logWrite(esc->log, "[PRIORITY] Processo PID %d finalizado\n", getPid(pcb));
      travaFila(esc->fila);
      esc->current_process = NULL;
      destravaFila(esc->fila);
   }
}

void *rotinaEscalonador(void *arg)
{
   Escalonador *esc = (Escalonador *)arg;

   switch (esc->tipo)
   {
   case POL_RR:
      escalonaRR(esc);
      break;
   case POL_PRIORITY:
      escalonaPrioridade(esc);
      break;
   case POL_FCFS:
   default:
      escalonaFCFS(esc);
      break;
   }

   return NULL;
}
