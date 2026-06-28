#include "escalonador.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

struct Escalonador
{
   TipoEscalonador tipo;
   FilaProntos *fila;
   int quantum_ms;

   PCB *current_process;
};

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

Escalonador *criaEscalonador(TipoEscalonador tipo, FilaProntos *fila, int quantum_ms)
{
   Escalonador *esc = calloc(1, sizeof(Escalonador));
   esc->tipo = tipo;
   esc->fila = fila;
   esc->quantum_ms = quantum_ms;
   esc->current_process = NULL;
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
      despacha(esc, pcb);
                                                         
      travaPcb(pcb);
      while (getEstado(pcb) != FINISHED)
         esperaPcb(pcb);
      destravaPcb(pcb);

      esc->current_process = NULL;
   }
}

static void escalonaRR(Escalonador *esc)
{
   /* TODO: implementar a preempcao por quantum. */
   escalonaFCFS(esc);
}

static void escalonaPrioridade(Escalonador *esc)
{
   escalonaFCFS(esc);
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
