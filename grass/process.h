#pragma once

#include "egos.h"
#include "syscall.h"
#include <stdbool.h>
#include <stddef.h>

enum proc_status {
    PROC_UNUSED,
    PROC_LOADING,
    PROC_READY,
    PROC_RUNNING,
    PROC_RUNNABLE,
    PROC_PENDING_SYSCALL,
    PROC_SLEEPING
};

#define MLFQ_NLEVELS          5
#define MAX_NPROCESS        16
#define SAVED_REGISTER_NUM  32
#define SAVED_REGISTER_SIZE SAVED_REGISTER_NUM * 4
#define SAVED_REGISTER_ADDR (void*)(EGOS_STACK_TOP - SAVED_REGISTER_SIZE)

struct process {
    int pid;
    struct syscall syscall;
    enum proc_status status;
    uint mepc, saved_registers[SAVED_REGISTER_NUM];
    /* Student's code goes here (Preemptive Scheduler | System Call). */

    // Lifecycle statistics
    uint t_created;
    uint t_started;
    uint t_finished;
    uint t_cpu;
    uint num_interrupts;
    bool has_started;

    // MLFQ fields
    int level;
    uint t_remaining;

    // Sleep and interrupt fields
    ulonglong latest_running_start_time;
    ulonglong wake_time;

    /* Student's code ends here. */
};

ulonglong mtime_get();

int proc_alloc();
void proc_free(int);
void proc_set_ready(int);
void proc_set_running(int);
void proc_set_runnable(int);
void proc_set_pending(int);

void mlfq_reset_level();
void mlfq_update_level(struct process* p, ulonglong runtime);
void proc_sleep(int pid, uint usec);
void proc_coresinfo();

extern uint core_to_proc_idx[NCORES];
