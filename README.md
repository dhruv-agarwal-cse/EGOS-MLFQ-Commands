# OS-Assignment (MLFQ Scheduler + User Commands)

## Team Member Names
- **Dhruv Agarwal (2024187)**
- **Akshit K B Bansal (2024058)**

**Group Number:** 18

---

## Contributions
- The design, logic, and overall MLFQ strategy were jointly planned.
- Kernel integration, debugging, interrupt tracing, and logging improvements were done collaboratively.
- Both members contributed to writing, running, and testing user processes.
- Final polishing, documentation, and cleanup were completed together.

---

## Overview
This assignment extends the EGOS-2000 Operating System by implementing:

1. **A fully functioning Multi-Level Feedback Queue (MLFQ) scheduler** within the kernel.  
2. **User-level programs** compiled into the OS filesystem.

The scheduler handles timer interrupts, CPU time accounting, level demotion/promotion, starvation prevention, and lifecycle statistics.  
The user programs run on top of this MLFQ scheduler without modifying the EGOS syscall surface.

---

## Core Components

### ✔ 1. MLFQ Scheduler (Kernel-Level)
Implemented inside:
- `kernel.c`
- `process.c`
- `process.h`

**Main functionalities:**
- Five priority queues (0–4).
- Each level has its own time quantum.
- Timer interrupts compute:
  - CPU time consumed (`t_cpu`)
  - Number of interrupts (`num_interrupts`)
  - Remaining quantum (`t_remaining`)
- Automatic **demotion** when quantum expires.
- Periodic **global reset/boost** to avoid starvation.
- Tracks:
  - creation time  
  - first-run time  
  - finish time  
  - turnaround time  
  - response time  
  - CPU usage  
  - number of interrupts  

### ✔ 2. Kernel Modifications
#### **kernel.c**
- Reads `mcause` to differentiate interrupt vs syscall.
- `intr_entry()` updated to:
  - compute runtime since last scheduled timestamp
  - log timer interrupts
  - update CPU stats
  - call `mlfq_update_level()`
- `proc_yield()` updated to:
  - perform MLFQ-based process selection
  - skip sleeping and non-runnable processes
  - handle pending syscalls
  - reset timer on every context switch

#### **process.c**
- Added new fields:
  - `level`
  - `t_cpu`
  - `t_remaining`
  - `latest_running_start_time`
  - `num_interrupts`
- Implemented:
  - `mlfq_update_level()`
  - `mlfq_reset_level()`
  - lifecycle bookkeeping

#### **process.h**
- Added MLFQ constants:
  - number of levels  
  - quantum sizes  
  - reset interval  
- Added fields in the PCB for stats.

---

## 3. User Process Demonstration
To validate scheduling, we used:

### ✔ CPU-bound user program
A looped computation with no syscalls:

- Triggers timer interrupts.
- Gets demoted through levels.
- Produces non-zero CPU time and interrupt counts.

### ✔ I/O-bound user program
A `my_printf()` heavy program:

- Frequently yields due to syscalls.
- Stays in higher queues.
- Demonstrates good interactivity.

---

## Lifecycle Statistics
Printed at termination:
[STATS] pid = X |
created = …
started = …
finished = …
turnaround = …
response = …
cpu time = …
interrupts = …

All values update correctly after MLFQ integration.

---

## Final Behavior Summary
- Timer interrupts now update CPU time correctly.
- Scheduler transitions print clean logs.
- User tasks run correctly under MLFQ priority rules.
- Starvation is prevented via periodic priority resets.
- System behaves identically to assignment expectations.

---

## Github Repo Link
https://github.com/dhruv-agarwal-cse/EGOS-MLFQ-Commands
