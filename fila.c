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

// HEAP FUNÇÕES LOCAIS
#define pai(i) (i - 1) / 2
#define esquerda(i) 2 * i + 1
#define direita(i) 2 * i + 2

static void trocaPCB(PCB **a, PCB **b) {
    PCB *temp = *a;
    *a = *b;
    *b = temp;
}

static void heapify_up(FilaProntos *h, int i) {
    while (i > 0 && getPriority(h->itens[pai(i)]) > getPriority(h->itens[i])) {
        trocaPCB(&h->itens[pai(i)], &h->itens[i]);
        i = pai(i);
    }
}

static void heapify_down(FilaProntos *h, int i) {
    int menor = i;
    int esq = esquerda(i);
    int dir = direita(i);

    if (esq < h->qtd && getPriority(h->itens[esq]) < getPriority(h->itens[menor]))
        menor = esq;
    if (dir < h->qtd && getPriority(h->itens[dir]) < getPriority(h->itens[menor]))
        menor = dir;

    if (menor != i) {
        trocaPCB(&h->itens[i], &h->itens[menor]);
        heapify_down(h, menor);
    }
}

static void heap_push(FilaProntos *h, PCB *PCB) {
    if (h->qtd >= h->capacidade) 
        return;
    h->itens[h->qtd] = PCB;
    heapify_up(h, h->qtd);
    h->qtd++;
}

static PCB *heap_pop(FilaProntos *h) {
    if (h->qtd == 0)
        return NULL;
    PCB *menor = h->itens[0];
    h->qtd--;
    h->itens[0] = h->itens[h->qtd];
    heapify_down(h, 0);
    return menor;
}


// Funções da fila de prontos

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

void insereHeap(FilaProntos *fila, PCB *pcb)
{
   pthread_mutex_lock(&fila->mutex);

   heap_push(fila, pcb);
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

PCB *esperaPrioritario(FilaProntos *fila)
{
   pthread_mutex_lock(&fila->mutex);

   while (fila->qtd == 0 && !fila->generator_done){
      pthread_cond_wait(&fila->cond, &fila->mutex);
   }

   PCB *pcb = heap_pop(fila);
   pthread_mutex_unlock(&fila->mutex);
   return pcb;
}

PCB *maiorPrioridade(FilaProntos *fila, PCB *atual)
{
   pthread_mutex_lock(&fila->mutex);
   PCB *res = heap_pop(fila);
   if(res == NULL) {
      pthread_mutex_unlock(&fila->mutex);
      return NULL;
   }
   else if(getPriority(res) >= getPriority(atual)){
      heap_push(fila, res);
      pthread_mutex_unlock(&fila->mutex);
      return NULL;
   }
   pthread_mutex_unlock(&fila->mutex);
   return res;
}