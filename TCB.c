#include "TCB.h"

struct TCB
{
    PCB *pcb;
    int thread_index;
};

TCB *criaTCB(PCB *pcb, int id){
    TCB *tcb = calloc(1, sizeof(TCB));
    tcb->pcb = pcb;
    tcb->thread_index = id;
    return tcb;
}