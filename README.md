# Simple Scheduler - README

## Overview
The **Simple Scheduler** is a multi-process scheduling system implemented in C. It demonstrates the use of **priority queues**, **signal handling**, **process management**, and **scheduling algorithms**. It supports multiple commands to manage processes, including submission, scheduling, history display, and statistics reporting. The scheduler uses **round-robin scheduling** with configurable time slices and CPU availability.

## Features
- **Multi-CPU Support**: Configure the number of CPUs for concurrent process execution.
- **Priority-based Scheduling**: Processes are scheduled based on assigned priority (1-4).
- **Time Slices**: Adjustable time quantum for each priority.
- **Process Commands**:
  - `submit`: Submit a new process with an optional priority.
  - `schedule`: Trigger process scheduling manually.
  - `stats`: Display average waiting and execution times for each priority.
  - `history`: Show details of all executed commands and their statistics.
- **Process Statistics**: Tracks waiting times, execution times, and durations for each process.

## Prerequisites
- **GCC Compiler**: To compile the program.
- **POSIX-compliant system**: For features like semaphores, signals, and process management.

## How to Compile
Use the following command to compile the program:
```bash
gcc -o simple_scheduler simple_scheduler.c -lpthread
```

## How to Run
Run the compiled program as follows:
```bash
./simple_scheduler
```

### Initial Configuration
On startup, you will be prompted to provide the following:
- `NCPU`: Number of CPUs available for concurrent process execution.
- `TSLICE`: Time quantum (in milliseconds) for process scheduling.

Example:
```
NCPU : 2
TSLICE : 10
```

## Commands
1. **submit**:
   - Submit a new process with an optional priority.
   - Example:
     ```bash
     submit <command> [priority]
     ```
     - `<command>`: The shell command to execute.
     - `[priority]`: Optional, default is 1 (highest priority is 4).

2. **schedule**:
   - Manually trigger the scheduler to run queued processes.
   - Example:
     ```bash
     schedule
     ```

3. **stats**:
   - Display job scheduling statistics (average waiting and execution times by priority).
   - Example:
     ```bash
     stats
     ```

4. **history**:
   - Show the history of all executed commands with their statistics.
   - Example:
     ```bash
     history
     ```

5. **Ctrl+C**:
   - Display all completed commands and exit the scheduler gracefully.

## Scheduler Algorithm
1. **Priority-Based Queue**:
   - Processes with higher priority are scheduled before lower-priority processes.
   - Processes of the same priority are scheduled in round-robin order.

2. **Time Quantum**:
   - Varies based on priority:
     - Priority 1: `TIME_QUANTUM`
     - Priority 2: `1.25 × TIME_QUANTUM`
     - Priority 3: `1.5 × TIME_QUANTUM`
     - Priority 4: `2 × TIME_QUANTUM`

3. **Preemption**:
   - Processes exceeding their time quantum are stopped and re-enqueued.

## Example Workflow
1. Submit processes:
   ```bash
   submit sleep 5 2
   submit ls 3
   ```
2. Trigger the scheduler:
   ```bash
   schedule
   ```
3. View statistics:
   ```bash
   stats
   ```
4. Check history:
   ```bash
   history
   ```

## File Structure
- `simple_scheduler.c`: Source code implementing the scheduler.

## Future Improvements
- Add dynamic process priority adjustment.
- Introduce support for additional scheduling policies (e.g., shortest job first).

## License
This project is licensed under the MIT License. Feel free to use, modify, and distribute.
