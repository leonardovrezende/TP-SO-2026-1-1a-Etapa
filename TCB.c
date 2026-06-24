#include "TCB.h"

struct TCB
{
    PCB *pcb;
    int thread_index;
};