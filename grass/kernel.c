/*
 * (C) 2025, Cornell University
 * All rights reserved.
 *
 * Description: kernel ≈ 2 handlers
 *   intr_entry() handles timer and device interrupts.
 *   excp_entry() handles system calls and faults (e.g., invalid memory access).
 */

#include "process.h"
#include <string.h>

uint core_in_kernel;
uint core_to_proc_idx[NCORES];
struct process proc_set[MAX_NPROCESS + 1];
/* proc_set[0] is a place holder for idle cores. */

#define curr_proc_idx core_to_proc_idx[core_in_kernel]
#define curr_pid      proc_set[curr_proc_idx].pid
#define curr_status   proc_set[curr_proc_idx].status
#define curr_saved    proc_set[curr_proc_idx].saved_registers

static void intr_entry(uint);
static void excp_entry(uint);

void kernel_entry() {
    /* With the kernel lock, only one core can enter this point at any time. */
    asm("csrr %0, mhartid" : "=r"(core_in_kernel));

    /* Save the process context. */
    asm("csrr %0, mepc" : "=r"(proc_set[curr_proc_idx].mepc));
    memcpy(curr_saved, SAVED_REGISTER_ADDR, SAVED_REGISTER_SIZE);

    uint mcause;
    asm("csrr %0, mcause" : "=r"(mcause));
    (mcause & (1 << 31)) ? intr_entry(mcause & 0x3FF) : excp_entry(mcause);

    /* Restore the process context. */
    asm("csrw mepc, %0" ::"r"(proc_set[curr_proc_idx].mepc));
    memcpy(SAVED_REGISTER_ADDR, curr_saved, SAVED_REGISTER_SIZE);
}

#define INTR_ID_TIMER   7
#define EXCP_ID_ECALL_U 8
#define EXCP_ID_ECALL_M 11
static void proc_yield();
static void proc_try_syscall(struct process* proc);

static void excp_entry(uint id) {
    if (id >= EXCP_ID_ECALL_U && id <= EXCP_ID_ECALL_M) {
        /* Copy the system call arguments from user space to the kernel. */
        uint syscall_paddr = earth->mmu_translate(curr_pid, SYSCALL_ARG);
        memcpy(&proc_set[curr_proc_idx].syscall, (void*)syscall_paddr,
               sizeof(struct syscall));
        proc_set[curr_proc_idx].syscall.status = PENDING;

        proc_set_pending(curr_pid);
        proc_set[curr_proc_idx].mepc += 4;
        proc_try_syscall(&proc_set[curr_proc_idx]);
        proc_yield();
        return;
    }
    /* Student's code goes here (System Call & Protection | Virtual Memory). */

    /* Kill the current process if curr_pid is a user application. */

    /* Student's code ends here. */
    FATAL("excp_entry: kernel got exception %d", id);
}

static void intr_entry(uint id) {
    if (id != INTR_ID_TIMER)
        FATAL("intr_entry: kernel got non-timer interrupt %d", id);

    /* Student's code goes here (Preemptive Scheduler). */

    /* Update the process lifecycle statistics. */
    struct process* p = &proc_set[curr_proc_idx];
    ulonglong current_time = mtime_get();

    // my_printf("[DEBUG] intr_entry: pid=%d status=%d latest_start=%d now=%d\n",
    //     (int)p->pid, (int)p->status,
    //     (int)p->latest_running_start_time, (int)current_time);


    if (p->status == PROC_RUNNING) {
        ulonglong running_time_on_cpu = current_time - p->latest_running_start_time;
        p->t_cpu += running_time_on_cpu;
        p->num_interrupts++;

        if (p->pid >= GPID_USER_START) {
            my_printf("[INTR timer interrupt | pid = %d | ran = %d | t_cpu = %d\n", (int)p->pid, (int)running_time_on_cpu, (int)p->t_cpu);
        }

        mlfq_update_level(p, running_time_on_cpu);
    }

    /* Student's code ends here. */
    proc_yield();
}

static void proc_yield() {
    if (curr_status == PROC_RUNNING) proc_set_runnable(curr_pid);

    /* Student's code goes here (Multiple Projects). */

    mlfq_reset_level(); // Calling reset level everytime the scheduler is invoked to check for starvation and pending keyboard inputs
    int next_idx = -1;
    bool is_found = false;

    for (int lvl = 0; lvl < MLFQ_NLEVELS && !is_found; lvl++) {
        for (uint i = 1; i <= MAX_NPROCESS; i++) {
            int idx = (curr_proc_idx + i) % MAX_NPROCESS;
            struct process* p = &proc_set[idx];

            if (p->status == PROC_PENDING_SYSCALL) // Trying to unblock pending syscalls 
                proc_try_syscall(p);

            if (p->status == PROC_SLEEPING) { // Checking if any sleeping processes can be woken up (Although this will never happen since sleep syscall is not handled, but adding this code here just because the comments said to do so)
                ulonglong now = mtime_get();
                if (now >= p->wake_time) {
                    p->status = PROC_RUNNABLE;
                } else {
                    continue;
                }
            }

            if (p->status != PROC_READY && p->status != PROC_RUNNABLE) // Skipping anything not runnable
                continue;

            if (p->pid < GPID_USER_START) { // Scheduling Kernel process first, no matter their level
                next_idx = idx;
                is_found = true;
                break;
            }

            if (p->level == lvl) { // Finding the first process for this level's queue
                next_idx = idx;
                is_found = true;
                break;
            }
        }
    }

    if (!is_found)
        FATAL("proc_yield: no runnable process");

    if (proc_set[next_idx].pid >= GPID_USER_START || curr_pid >= GPID_USER_START) {
        my_printf("[SCHED] Switching from pid = %d ==> pid = %d (in level = %d)\n", (int)curr_pid, (int)proc_set[next_idx].pid, (int)proc_set[next_idx].level);
    }

    /* Student's code ends here. */

    curr_proc_idx = next_idx;
    earth->mmu_switch(curr_pid);
    earth->mmu_flush_cache();
    if (curr_status == PROC_READY) {
        /* Setup argc, argv and program counter for a newly created process. */
        curr_saved[0]                = APPS_ARG;
        curr_saved[1]                = APPS_ARG + 4;
        proc_set[curr_proc_idx].mepc = APPS_ENTRY;
    }
    proc_set_running(curr_pid);
    earth->timer_reset(core_in_kernel);
}

static void proc_try_send(struct process* sender) {
    for (uint i = 0; i < MAX_NPROCESS; i++) {
        struct process* dst = &proc_set[i];
        if (dst->pid == sender->syscall.receiver &&
            dst->status != PROC_UNUSED) {
            /* Return if dst is not receiving or not taking msg from sender. */
            if (!(dst->syscall.type == SYS_RECV &&
                  dst->syscall.status == PENDING))
                return;
            if (!(dst->syscall.sender == GPID_ALL ||
                  dst->syscall.sender == sender->pid))
                return;

            dst->syscall.status = DONE;
            dst->syscall.sender = sender->pid;
            /* Copy the system call arguments within the kernel PCB. */
            memcpy(dst->syscall.content, sender->syscall.content,
                   SYSCALL_MSG_LEN);
            return;
        }
    }
    FATAL("proc_try_send: unknown receiver pid=%d", sender->syscall.receiver);
}

static void proc_try_recv(struct process* receiver) {
    if (receiver->syscall.status == PENDING) return;

    /* Copy the system call struct from the kernel back to user space. */
    uint syscall_paddr = earth->mmu_translate(receiver->pid, SYSCALL_ARG);
    memcpy((void*)syscall_paddr, &receiver->syscall, sizeof(struct syscall));

    /* Set the receiver and sender back to RUNNABLE. */
    proc_set_runnable(receiver->pid);
    proc_set_runnable(receiver->syscall.sender);
}

static void proc_try_syscall(struct process* proc) {
    switch (proc->syscall.type) {
    case SYS_RECV:
        proc_try_recv(proc);
        break;
    case SYS_SEND:
        proc_try_send(proc);
        break;
    default:
        FATAL("proc_try_syscall: unknown syscall type=%d", proc->syscall.type);
    }
}
