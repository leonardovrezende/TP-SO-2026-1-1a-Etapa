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

PCB *leProcesso(FILE *f)
{
   PCB *pcb = calloc(1, sizeof(PCB));
   pcb->remaining_time = 0;
   fscanf(f, "%d", &pcb->process_len);
   fscanf(f, "%d", &pcb->priority);
   fscanf(f, "%d", &pcb->num_threads);
   fscanf(f, "%d", &pcb->start_time);
   inicializaMutexCond(pcb);
   return pcb;
}

void liberaProcesso(PCB *pcb)
{
   free(pcb);
}

void inicializaMutexCond(PCB *pcb)
{
   pthread_mutex_init(&pcb->mutex, NULL);
   pthread_cond_init(&pcb->cv, NULL);
}