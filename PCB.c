#include "PCB.h"

typedef enum
{
   READY,
   RUNNING,
   FINISHED
} ProcessState;

struct PCB
{
   int pid;
   int process_len;
   int remaining_time;
   int priority;
   int num_threads;
   int start_time;

   ProcessState state;
   pthread_mutex_t mutex;
   pthread_cond_t cv;
   pthread_t *thread_ids;
};

static void inicializaProcesso(PCB *pcb)
{
   pthread_mutex_init(&pcb->mutex, NULL);
   pthread_cond_init(&pcb->cv, NULL);
   pcb->remaining_time = pcb->process_len;
   pcb->state = READY;
   pcb->thread_ids = calloc(pcb->num_threads, sizeof(pthread_t));
}

PCB *leProcesso(FILE *f)
{
   PCB *pcb = calloc(1, sizeof(PCB));
   fscanf(f, "%d", &pcb->process_len);
   fscanf(f, "%d", &pcb->priority);
   fscanf(f, "%d", &pcb->num_threads);
   fscanf(f, "%d", &pcb->start_time);
   inicializaProcesso(pcb);

   return pcb;
}

void liberaProcesso(PCB *pcb)
{
   free(pcb);
}

static void *routine(void *arg){
   PCB *pcb = (PCB*) arg;

   pthread_mutex_lock(&pcb->mutex);
   while(pcb->state != RUNNING){
      pthread_cond_wait(&pcb->cv, &pcb->mutex);
   }
   pthread_mutex_unlock(&pcb->mutex);

   return NULL;
}

TCB *criaThread(PCB *pcb, int tcbId){
      pthread_create(&pcb->thread_ids[tcbId], NULL, &routine, (void*)pcb);
      return criaTCB(pcb, tcbId);
}

int getNumThreads(PCB *pcb){
   return pcb->num_threads;
}