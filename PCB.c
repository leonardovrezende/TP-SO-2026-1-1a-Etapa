#include "PCB.h"
#include "TCB.h"
#include <unistd.h>

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

static void inicializaProcesso(PCB *pcb, int i)
{
   pcb->pid = i;
   pthread_mutex_init(&pcb->mutex, NULL);
   pthread_cond_init(&pcb->cv, NULL);
   pcb->remaining_time = pcb->process_len;
   pcb->state = READY;
   pcb->thread_ids = calloc(pcb->num_threads, sizeof(pthread_t));
}

PCB *leProcesso(FILE *f, int i)
{
   PCB *pcb = calloc(1, sizeof(PCB));
   fscanf(f, "%d", &pcb->process_len);
   fscanf(f, "%d", &pcb->priority);
   fscanf(f, "%d", &pcb->num_threads);
   fscanf(f, "%d", &pcb->start_time);
   inicializaProcesso(pcb, i);

   return pcb;
}

void liberaProcesso(PCB *pcb)
{
   free(pcb->thread_ids);
   free(pcb);
}

static void *routine(void *arg){
   PCB *pcb = (PCB*) arg;

   pthread_mutex_lock(&pcb->mutex);
   while(pcb->state != RUNNING && pcb->state != FINISHED){
      pthread_cond_wait(&pcb->cv, &pcb->mutex);
   }
   if(pcb->state == FINISHED){
      pthread_mutex_unlock(&pcb->mutex);
      return NULL;
   }
   pthread_mutex_unlock(&pcb->mutex);

   usleep(pcb->process_len/pcb->num_threads * 1000); //fiquei na duvida sobre como determinar o tempo de execucao da thread. pensei assim.

   pthread_mutex_lock(&pcb->mutex);
   pcb->remaining_time -= pcb->process_len/pcb->num_threads * 1000;
   //se tempo do processo acabou, muda estado
   if(pcb->remaining_time <= 0){
      pcb->state = FINISHED;
      pthread_cond_broadcast(&pcb->cv);
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

int getStartTime(PCB *pcb){
   return pcb->start_time;
}

int getPid(PCB *pcb){
   return pcb->pid;
}

int getPriority(PCB *pcb){
   return pcb->priority;
}

int getRemainingTime(PCB *pcb){
   return pcb->remaining_time;
}

ProcessState getEstado(PCB *pcb){
   return pcb->state;
}

void setEstado(PCB *pcb, ProcessState estado){
   pcb->state = estado;
}

void travaPcb(PCB *pcb){
   pthread_mutex_lock(&pcb->mutex);
}

void destravaPcb(PCB *pcb){
   pthread_mutex_unlock(&pcb->mutex);
}

void esperaPcb(PCB *pcb){
   pthread_cond_wait(&pcb->cv, &pcb->mutex);
}

void sinalizaPcb(PCB *pcb){
   pthread_cond_broadcast(&pcb->cv);
}

void joinThreads(PCB *pcb){
   for(int i = 0; i < pcb->num_threads; i++)
      pthread_join(pcb->thread_ids[i], NULL);
}

int ordenaPCB(const void *arg1, const void *arg2){
   PCB *proc1 = *(PCB**) arg1;
   PCB *proc2 = *(PCB**) arg2;

   return proc1->start_time - proc2->start_time;
}