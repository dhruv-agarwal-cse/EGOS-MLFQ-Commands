/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: helper functions for process management
 */

#include "process.h"

#define MLFQ_NLEVELS          5
#define MLFQ_RESET_PERIOD     10000000         /* 10 seconds */
#define MLFQ_LEVEL_RUNTIME(x) (x + 1) * 100000 /* e.g., 100ms for level 0 */
extern struct process proc_set[MAX_NPROCESS + 1];

static void proc_set_status(int pid, enum proc_status status) {
    for (uint i = 0; i < MAX_NPROCESS; i++)
        if (proc_set[i].pid == pid) proc_set[i].status = status;
}

void proc_set_ready(int pid) { proc_set_status(pid, PROC_READY); }

void proc_set_running(int pid) {
    proc_set_status(pid, PROC_RUNNING);
    
    for (uint i = 0; i < MAX_NPROCESS; i++) {
        if (proc_set[i].pid == pid && proc_set[i].status != PROC_UNUSED) { // If this is the first time the process is running, then set t_started
            if (!proc_set[i].has_started) {
                proc_set[i].has_started = true;
                proc_set[i].t_started = (uint) mtime_get();
            }
            proc_set[i].latest_running_start_time = mtime_get();
            
            break;
        }
    }
}

void proc_set_runnable(int pid) { proc_set_status(pid, PROC_RUNNABLE); }

void proc_set_pending(int pid) { proc_set_status(pid, PROC_PENDING_SYSCALL); }

int proc_alloc() {
    static uint curr_pid = 0;
    for (uint i = 1; i <= MAX_NPROCESS; i++)
        if (proc_set[i].status == PROC_UNUSED) {
            proc_set[i].pid    = ++curr_pid;
            proc_set[i].status = PROC_LOADING;
            /* Student's code goes here (Preemptive Scheduler | System Call). */
            
            /* Initialization of lifecycle statistics, MLFQ or process sleep. */
            proc_set[i].t_created      = (uint) mtime_get();
            proc_set[i].t_started      = 0;
            proc_set[i].t_finished     = 0;
            proc_set[i].t_cpu          = 0;
            proc_set[i].num_interrupts = 0;
            proc_set[i].has_started    = false;

            proc_set[i].level = 0;
            proc_set[i].t_remaining = MLFQ_LEVEL_RUNTIME(0);
            
            /* Student's code ends here. */
            return curr_pid;
        }

    FATAL("proc_alloc: reach the limit of %d processes", MAX_NPROCESS);
}

void proc_free(int pid) {
    /* Student's code goes here (Preemptive Scheduler). */

    /* Record finish time + print statistics */
    ulonglong now = mtime_get();

    if (pid != GPID_ALL) {
        // Iterating over the process set to find the process with the given pid
        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].pid == pid && proc_set[i].status != PROC_UNUSED) {
                proc_set[i].t_finished = (uint) now;
                uint created   = proc_set[i].t_created;
                uint started   = proc_set[i].t_started;
                uint finished  = proc_set[i].t_finished;
                uint turnaround = (finished - created);
                uint response   = (started - created);
                uint cpu        = proc_set[i].t_cpu;
                uint interrupts       = proc_set[i].num_interrupts;

                my_printf("[STATS] pid=%d created=%u started=%u finished=%u\n", pid, created, started, finished);
                my_printf("[STATS] pid=%d turnaround=%u response=%u cpu=%u interrupts=%u\n", pid, turnaround, response, cpu, interrupts);
                break;
            }
        }

        earth->mmu_free(pid);
        proc_set_status(pid, PROC_UNUSED);
    } else {
        // Printing stats for all processes in the process set
        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].pid >= GPID_USER_START && proc_set[i].status != PROC_UNUSED) {
                proc_set[i].t_finished = (uint) now;
                uint p_id      = proc_set[i].pid;
                uint created   = proc_set[i].t_created;
                uint started   = proc_set[i].t_started;
                uint finished  = proc_set[i].t_finished;
                uint turnaround = finished - created;
                uint response   = started - created;
                uint cpu        = proc_set[i].t_cpu;
                uint intr       = proc_set[i].num_interrupts;

                my_printf("[STATS] pid=%d created=%u started=%u finished=%u\n", p_id, created, started, finished);
                my_printf("[STATS] pid=%d turnaround=%u response=%u cpu=%u interrupts=%u\n", p_id, turnaround, response, cpu, intr);
            }
        }
        
        /* Free all user processes. */
        for (uint i = 0; i < MAX_NPROCESS; i++)
            if (proc_set[i].pid >= GPID_USER_START &&
                proc_set[i].status != PROC_UNUSED) {
                earth->mmu_free(proc_set[i].pid);
                proc_set[i].status = PROC_UNUSED;
            }
    }
    /* Student's code ends here. */
}

void mlfq_update_level(struct process* p, ulonglong runtime) {
    /* Student's code goes here (Preemptive Scheduler). */

    /* Update the MLFQ-related fields in struct process* p after this
     * process has run on the CPU for another runtime microseconds. */

    if (p == NULL) return;

    if (p->pid >= GPID_USER_START) {
        my_printf("[MLFQ] pid=%d runtime=%d before update: level=%d t_remaining=%d\n", (int)p->pid, (int)runtime, (int)p->level, (int)p->t_remaining);
    }

    if (runtime >= p->t_remaining) {
        if (p->level < MLFQ_NLEVELS - 1) {
            p->level++;
        }
        p->t_remaining = MLFQ_LEVEL_RUNTIME(p->level);
    } else {
        p->t_remaining -= runtime;
    }

    if (p->pid >= GPID_USER_START) {
        my_printf("[MLFQ] pid=%d after update: level=%d t_remaining=%d\n", (int)p->pid, (int)p->level, (int)p->t_remaining);
    }

    /* Student's code ends here. */
}

void mlfq_reset_level() {
    /* Student's code goes here (Preemptive Scheduler). */
    
    static ulonglong MLFQ_last_reset_time = 0;
    ulonglong current_time = mtime_get();
    
    /* Reset the level of GPID_SHELL if there is pending keyboard input. */
    if (!earth->tty_input_empty()) {
        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].pid == GPID_SHELL && proc_set[i].status != PROC_UNUSED) {
                proc_set[i].level = 0;
                proc_set[i].t_remaining = MLFQ_LEVEL_RUNTIME(0);
                if (proc_set[i].pid >= GPID_USER_START) {
                    my_printf("[MLFQ] Shell priority boosted due to keyboard input.\n");
                }
                break;
            }
        }
    }

    /* Reset the level of all processes every MLFQ_RESET_PERIOD microseconds. */
    if (current_time - MLFQ_last_reset_time >= MLFQ_RESET_PERIOD) { // More than RESET_PERIOD has passed
        MLFQ_last_reset_time = current_time;

        for (uint i = 0; i < MAX_NPROCESS; i++) {
            if (proc_set[i].status != PROC_UNUSED && proc_set[i].pid >= GPID_USER_START) {
                proc_set[i].level = 0;
                proc_set[i].t_remaining = MLFQ_LEVEL_RUNTIME(0);
                my_printf("[MLFQ] Periodic reset: pid=%d moved to level 0\n", (int)proc_set[i].pid);
            }
        }
    }

    /* Student's code ends here. */
}

void proc_sleep(int pid, uint usec) {
    /* Student's code goes here (System Call & Protection). */
    
    for (uint i = 0; i < MAX_NPROCESS; i++) {
        if (proc_set[i].pid == pid && proc_set[i].status != PROC_UNUSED) {
            
            ulonglong now = mtime_get();
            proc_set[i].wake_time = now + usec;
            proc_set[i].status = PROC_SLEEPING;

            return;
        }
    }
    
    /* Student's code ends here. */
}

void proc_coresinfo() {
    /* Student's code goes here (Multicore & Locks). */

    /* Print out the pid of the process running on each CPU core. */

    /* Student's code ends here. */
}
