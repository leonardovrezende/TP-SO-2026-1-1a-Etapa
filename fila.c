#include "fila.h"
#include <pthread.h>
#include <stdlib.h>

struct FilaProntos
{
   PCB **itens;
   int capacidade;
   int inicio;
   int qtd;

   int generator_done;
   pthread_mutex_t mutex;
   pthread_cond_t cond;
};

FilaProntos *criaFila(int capacidade)
{
   if (capacidade < 1)
      capacidade = 1;

   FilaProntos *fila = calloc(1, sizeof(FilaProntos));
   fila->itens = calloc(capacidade, sizeof(PCB *));
   fila->capacidade = capacidade;
   fila->inicio = 0;
   fila->qtd = 0;
   fila->generator_done = 0;
   pthread_mutex_init(&fila->mutex, NULL);
   pthread_cond_init(&fila->cond, NULL);

   return fila;
}

void liberaFila(FilaProntos *fila)
{
   pthread_mutex_destroy(&fila->mutex);
   pthread_cond_destroy(&fila->cond);
   free(fila->itens);
   free(fila);
}

static void cresceFila(FilaProntos *fila)
{
   int nova_cap = fila->capacidade * 2;
   PCB **novos = calloc(nova_cap, sizeof(PCB *));
   for (int i = 0; i < fila->qtd; i++)
      novos[i] = fila->itens[(fila->inicio + i) % fila->capacidade];

   free(fila->itens);
   fila->itens = novos;
   fila->capacidade = nova_cap;
   fila->inicio = 0;
}

void insereFila(FilaProntos *fila, PCB *pcb)
{
   pthread_mutex_lock(&fila->mutex);
   if (fila->qtd == fila->capacidade)
      cresceFila(fila);

   int pos = (fila->inicio + fila->qtd) % fila->capacidade;
   fila->itens[pos] = pcb;
   fila->qtd++;

   pthread_cond_signal(&fila->cond);
   pthread_mutex_unlock(&fila->mutex);
}

static PCB *removeInicio(FilaProntos *fila)
{
   if (fila->qtd == 0)
      return NULL;

   PCB *pcb = fila->itens[fila->inicio];
   fila->itens[fila->inicio] = NULL;
   fila->inicio = (fila->inicio + 1) % fila->capacidade;
   fila->qtd--;
   return pcb;
}

PCB *removeFila(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);
   PCB *pcb = removeInicio(fila);
   pthread_mutex_unlock(&fila->mutex);
   return pcb;
}

PCB *removePcbFila(FilaProntos *fila, PCB *pcb)
{
   pthread_mutex_lock(&fila->mutex);

   PCB *removido = NULL;
   for (int i = 0; i < fila->qtd; i++)
   {
      int idx = (fila->inicio + i) % fila->capacidade;
      if (fila->itens[idx] == pcb)
      {
         removido = pcb;
         /* desloca os elementos seguintes uma posicao para tras */
         for (int j = i; j < fila->qtd - 1; j++)
         {
            int a = (fila->inicio + j) % fila->capacidade;
            int b = (fila->inicio + j + 1) % fila->capacidade;
            fila->itens[a] = fila->itens[b];
         }
         int ultimo = (fila->inicio + fila->qtd - 1) % fila->capacidade;
         fila->itens[ultimo] = NULL;
         fila->qtd--;
         break;
      }
   }

   pthread_mutex_unlock(&fila->mutex);
   return removido;
}

int filaVazia(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);
   int vazia = (fila->qtd == 0);
   pthread_mutex_unlock(&fila->mutex);
   return vazia;
}

int tamanhoFila(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);
   int qtd = fila->qtd;
   pthread_mutex_unlock(&fila->mutex);
   return qtd;
}

void marcaGeradorPronto(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);
   fila->generator_done = 1;
   pthread_cond_broadcast(&fila->cond);
   pthread_mutex_unlock(&fila->mutex);
}

int geradorPronto(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);
   int pronto = fila->generator_done;
   pthread_mutex_unlock(&fila->mutex);
   return pronto;
}

static PCB *removeIndex(FilaProntos *fila, int index)
{
   int abs = (fila->inicio + index) % fila->capacidade;
   PCB *pcb = fila->itens[abs];
   for (int i = index; i < fila->qtd - 1; i++)
   {
      int a = (fila->inicio + i) % fila->capacidade;
      int b = (fila->inicio + i + 1) % fila->capacidade;
      fila->itens[a] = fila->itens[b];
   }
   int ultimo = (fila->inicio + fila->qtd - 1) % fila->capacidade;
   fila->itens[ultimo] = NULL;
   fila->qtd--;
   return pcb;
}

PCB *esperaProximo(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);

   while (fila->qtd == 0 && !fila->generator_done)
      pthread_cond_wait(&fila->cond, &fila->mutex);

   PCB *pcb = removeInicio(fila);
   pthread_mutex_unlock(&fila->mutex);
   return pcb;
}

static PCB *maiorPrioridadeLocked(FilaProntos *fila, PCB *atual)
{
   if (atual == NULL)
      return NULL;

   int prioAtual = getPriority(atual);
   int melhor_rel = -1;
   int menor_prio = prioAtual;

   for (int i = 0; i < fila->qtd; i++)
   {
      int pos = (fila->inicio + i) % fila->capacidade;
      PCB *cand = fila->itens[pos];
      int prioTeste = getPriority(cand);
      if (prioTeste < menor_prio)
      {
         menor_prio = prioTeste;
         melhor_rel = i;
      }
   }

   PCB *prox = NULL;
   if (melhor_rel >= 0)
      prox = removeIndex(fila, melhor_rel);

   return prox;
}

PCB *esperaPrioritario(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);

   while (fila->qtd == 0 && !fila->generator_done)
      pthread_cond_wait(&fila->cond, &fila->mutex);

   PCB *pcb = fila->itens[fila->inicio];
   PCB *comp = maiorPrioridadeLocked(fila, pcb);
   if (comp != NULL)
      pcb = comp;
   else
   {
      fila->itens[fila->inicio] = NULL;
      fila->inicio = (fila->inicio + 1) % fila->capacidade;
      fila->qtd--;
   }
   pthread_mutex_unlock(&fila->mutex);
   return pcb;
}

PCB *maiorPrioridade(FilaProntos *fila, PCB *atual)
{
   pthread_mutex_lock(&fila->mutex);
   PCB *res = maiorPrioridadeLocked(fila, atual);
   pthread_mutex_unlock(&fila->mutex);
   return res;
}