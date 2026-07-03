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

static const char *rotulo(TipoEscalonador tipo)
{
   switch (tipo)
   {
   case POL_FCFS:
      return "FCFS";
   case POL_RR:
      return "RR";
   case POL_PRIORITY:
      return "PRIORITY";
   default:
      return "?";
   }
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
   return esc;
}

void liberaEscalonador(Escalonador *esc)
{
   free(esc);
}

static void despacha(Escalonador *esc, PCB *pcb)
{
   travaPcb(pcb);
   setEstado(pcb, RUNNING);
   esc->current_process = pcb;
   sinalizaPcb(pcb); 
   destravaPcb(pcb);
}

static void escalonaFCFS(Escalonador *esc)
{
   PCB *pcb;
   while ((pcb = esperaProximo(esc->fila)) != NULL)
   {
      if(NUM_CPUS == 2) logWrite(esc->log, "[FCFS] Executando processo PID %d // processador %d\n", getPid(pcb), esc->cpu_id);
      else logWrite(esc->log, "[FCFS] Executando processo PID %d\n", getPid(pcb));

      while (getEstado(pcb) != FINISHED) {
         despacha(esc, pcb);
         travaPcb(pcb);
         while (getEstado(pcb) == RUNNING)
            esperaPcb(pcb); /* aguarda threads sinalizarem READY ou FINISHED */
         destravaPcb(pcb);
      }

      logWrite(esc->log, "[FCFS] Processo PID %d finalizado\n", getPid(pcb));
      esc->current_process = NULL;
   }
}

static void escalonaRR(Escalonador *esc)
{
   PCB *pcb;
   while ((pcb = esperaProximo(esc->fila)) != NULL)
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
      esc->current_process = NULL;
   }
}

static void escalonaPrioridade(Escalonador *esc)
{
   PCB *pcb;
   while ((pcb = esperaPrioritario(esc->fila)) != NULL)
   {
      if(NUM_CPUS == 2) logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d // processador %d\n",
              getPid(pcb), getPriority(pcb), esc->cpu_id);
      else logWrite(esc->log, "[PRIORITY] Executando processo PID %d prioridade %d\n",
              getPid(pcb), getPriority(pcb));

      while (getEstado(pcb) != FINISHED)
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
               insereFila(esc->fila, pcb);
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

         if (preempted)
            continue;

         if (getEstado(pcb) == FINISHED)
         {
            destravaPcb(pcb);
            break;
         }

         destravaPcb(pcb);
      }

      logWrite(esc->log, "[PRIORITY] Processo PID %d finalizado\n", getPid(pcb));
      esc->current_process = NULL;
   }
}

void *rotinaEscalonador(void *arg)
{
   Escalonador *esc = (Escalonador *)arg;

   //esc->log_file = fopen("log_execucao_minikernel.txt", "w");

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

   //logWrite(esc->log, "Escalonador terminou execução de todos processos\n");
   //fclose(esc->log);
   //esc->log = NULL;

   return NULL;
}
