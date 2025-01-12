#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <semaphore.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_HISTORY_SIZE 50
int MAX_CPU = 1;
int TIME_QUANTUM = 10;

typedef struct
{
    char *command;
    char *args;
    int pid;
    int priority;
    struct timeval start_time, end_time;
    double waiting_time;   // New field to store total waiting time
    double duration;
} history;
history command_history[MAX_INPUT_SIZE];

struct process
{
    int pid;
    int priority;
    struct process *next;
    struct timeval last_stop_time;  // Track when process was last stopped
    struct timeval last_start_time; // Track when process was last started
} typedef process;

struct priorityqueue
{
    process *front;
    process *rear;
} typedef pque;

volatile sig_atomic_t time_quantum_expired = 0;
void timerHandler(int sig_num)
{
    time_quantum_expired = 1;
}

// Function to format time into HH:MM:SS
char *formatTime(long long timestamp) {
    struct tm tm_info;
    time_t time_sec = (time_t)timestamp;
    localtime_r(&time_sec, &tm_info);
    char *time_str = (char *)malloc(9); // Allocate 9 bytes for HH:MM:SS
    strftime(time_str, 9, "%H:%M:%S", &tm_info);
    return time_str;
}

int command_num = 0;
void display_statistics() {
    double total_waiting_time[5] = {0}, total_duration[5] = {0};
    int priority_counts[5] = {0};
    
    // Aggregate waiting time and duration based on priority
    for (int i = 0; i < command_num; i++) {
        int priority = command_history[i].priority;
        if (priority >= 1 && priority <= 4) {
            total_waiting_time[priority] += command_history[i].waiting_time;
            total_duration[priority] += command_history[i].duration;
            priority_counts[priority]++;
        }
    }

    // Display averages in table format
    printf("\nJob Scheduling Statistics:\n");
    printf("---------------------------------------------------------\n");
    printf("| Priority | Average Waiting Time (ms) | Average Execution Time (ms) |\n");
    printf("---------------------------------------------------------\n");

    for (int priority = 1; priority <= 4; priority++) {
        if (priority_counts[priority] > 0) {
            double avg_waiting_time = total_waiting_time[priority] / priority_counts[priority];
            double avg_duration = total_duration[priority] / priority_counts[priority];
            printf("|    %d     |        %.2f           |         %.2f            |\n", priority, avg_waiting_time, avg_duration);
        }
    }
    
    printf("---------------------------------------------------------\n");
}


void sigintHandler(int sig_num)
{
    printf("Exiting group50@SimpleShell\n\n");

    for (int i = 0; i < command_num; i++)
    {
        char *start_time_str = formatTime(command_history[i].start_time.tv_sec);
        char *end_time_str = formatTime(command_history[i].end_time.tv_sec);

        printf("Command: %s\n", command_history[i].command);
        printf("Args: %s\n", command_history[i].args);
        printf("Pid: %i\n", command_history[i].pid);
        printf("Start_Time: %s.%06ld\n", start_time_str, command_history[i].start_time.tv_usec);
        printf("End_Time: %s.%06ld\n", end_time_str, command_history[i].end_time.tv_usec);
        printf("Duration: %f ms\n", command_history[i].duration);
        printf("Waiting Time: %f ms\n", command_history[i].waiting_time);
        printf("\n");

        free(start_time_str);
        free(end_time_str);
    }

    exit(0);
}

int start_sche = 0;
void sigQuitHandler(int sig_num)
{
    start_sche = 1;
}

void parse_input(char *input, char *command, char *args[])
{
    char *token = strtok(input, " \t\n");
    int argument_count = 0;

    while (token != NULL)
    {
        while (argument_count < MAX_ARGS - 1)
        {
            args[argument_count] = token;
            argument_count++;
            token = strtok(NULL, " \t\n");
        }
    }

    args[argument_count] = NULL; // Null-terminate the argument list
    strcpy(command, args[0]);
}

pid_t scheduler_pid = -1;
sem_t *scheduler_sem;

int create_a_new_process(char *command)
{
    // Fork a child process
    pid_t pid = fork();
    return pid;
}

int launch(char *command)
{
    // Creates a new process by calling create_a_new_process() method
    int status;
    status = create_a_new_process(command);
    return status;
}

void enqueue(int pid, int priority, pque *readyQueue) {
    process *temp = (process *)malloc(sizeof(process));
    if (temp == NULL) {
        fprintf(stderr, "Memory allocation failed for process");
        exit(EXIT_FAILURE);
    }
    temp->pid = pid;
    temp->priority = priority;
    gettimeofday(&temp->last_stop_time, NULL);
    temp->next = NULL;
    if (readyQueue->front == NULL) {
        readyQueue->front = temp;
        readyQueue->rear = temp;
    } else {
        process *cntr = readyQueue->front;
        if (temp->priority > cntr->priority) {
            temp->next = cntr;
            readyQueue->front = temp;
        } else {
            while ((cntr->next != NULL) && (cntr->next->priority >= temp->priority)) {
                cntr = cntr->next;
            }
            temp->next = cntr->next;
            cntr->next = temp;
            if (temp->next == NULL) {
                readyQueue->rear = temp;
            }
        }
    }
}

process *dequeue(pque *readyQueue) {
    if (readyQueue->front == NULL) {
        return NULL;
    }
    process *temp = readyQueue->front;
    readyQueue->front = readyQueue->front->next;
    return temp;
}

int calculate_time_slice(int priority) {
    if(priority == 1){
        return TIME_QUANTUM * 1;
    }
    if(priority == 2){
        return TIME_QUANTUM * 1.25;
    }
    if(priority == 3){
        return TIME_QUANTUM * 1.5;
    }
    if(priority == 4){
        return TIME_QUANTUM * 2;
    }
    return -1; 
}

void schedule_the_Processes(pque *readyQueue, process *runningqueue[MAX_CPU]) {
    signal(SIGALRM, timerHandler); // Set up handler for time slice

    while (readyQueue->front != NULL) {
        int runningProcesses = 0;

        // Run up to MAX_CPU processes concurrently
        while (runningProcesses < MAX_CPU && readyQueue->front != NULL) {
            runningqueue[runningProcesses] = dequeue(readyQueue);
            
            struct timeval start;
            gettimeofday(&start, NULL);

            for (int j = 0; j < command_num; j++) {
                if (command_history[j].pid == runningqueue[runningProcesses]->pid) {
                    double wait_interval = 
                        (double)(start.tv_sec - runningqueue[runningProcesses]->last_stop_time.tv_sec) * 1000.0 +
                        (double)(start.tv_usec - runningqueue[runningProcesses]->last_stop_time.tv_usec) / 1000.0;
                    command_history[j].waiting_time += wait_interval;
                    command_history[j].start_time = start; // Update start time for execution tracking
                    break;
                }
            }
            gettimeofday(&runningqueue[runningProcesses]->last_start_time, NULL);
            kill(runningqueue[runningProcesses]->pid, SIGCONT); // Resume process if stopped
            runningProcesses++;
        }

        time_quantum_expired = 0;

        // Calculate time slice based on priority
        int time_slice = calculate_time_slice(runningqueue[0]->priority); 
        alarm(time_slice);

        // Wait for either the time slice to expire or process completion
        while (!time_quantum_expired && runningProcesses > 0) {
            for (int i = 0; i < runningProcesses; i++) {
                int status;
                pid_t result = waitpid(runningqueue[i]->pid, &status, WNOHANG);

                if (result == -1) {
                    runningProcesses--;
                } else if (result > 0) {
                    if (WIFEXITED(status) || WIFSIGNALED(status)) {
                        struct timeval end;
                        gettimeofday(&end, NULL);
                        runningProcesses--;
                        printf("Process finished with pid %d\n", runningqueue[i]->pid);
                        for (int j = 0; j < command_num; j++) {
                            if (command_history[j].pid == runningqueue[i]->pid) {
                                command_history[j].end_time = end;
                                command_history[j].duration = (double)(end.tv_sec - command_history[j].start_time.tv_sec) * 1000.0 +
                                                              (double)(end.tv_usec - command_history[j].start_time.tv_usec) / 1000.0;
                                break;
                            }
                        }
                    } else if (WIFSTOPPED(status)) {
                        gettimeofday(&runningqueue[i]->last_stop_time, NULL);
                        enqueue(runningqueue[i]->pid, runningqueue[i]->priority, readyQueue);
                        printf("Process with pid %d preempted and re-enqueued\n", runningqueue[i]->pid);
                    }
                }
            }
        }

        // Preempt any remaining running processes as time quantum expired
        if (time_quantum_expired) {
            for (int i = 0; i < runningProcesses; i++) {
                if (kill(runningqueue[i]->pid, SIGSTOP) == 0) {
                    gettimeofday(&runningqueue[i]->last_stop_time, NULL);
                    enqueue(runningqueue[i]->pid, runningqueue[i]->priority, readyQueue);
                    printf("Process with pid %d preempted and re-enqueued\n", runningqueue[i]->pid);
                }
            }
        }
    }
}

void simple_scheduler_daemon(pque *readyQueue, process *runningqueue[MAX_CPU]) {
    while (1) {
        sem_wait(scheduler_sem);
        schedule_the_Processes(readyQueue, runningqueue);
        sem_post(scheduler_sem);
        usleep(1000); // Sleep for a short time to reduce CPU usage
    }
}

struct timeval start_time, end_time;
int main()
{
    char c;
    printf("NCPU : ");
    scanf("%d", &MAX_CPU);
    printf("TSLICE : ");
    scanf("%d", &TIME_QUANTUM);
    scanf("%c", &c);

    char input[MAX_INPUT_SIZE];
    char command[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];

    double duration;
    process *runnQueue[MAX_CPU];
    int priority = 1;
    pque *readyQueue = (pque *)malloc(sizeof(pque));
    if (readyQueue == NULL){
        fprintf(stderr, "merrory allocation for ready Queue failed");
        exit(EXIT_FAILURE);
    }

    scheduler_pid = fork();
    if (scheduler_pid == 0) {
        // Child process (SimpleScheduler daemon)
        simple_scheduler_daemon(readyQueue, runnQueue);
        exit(0);
    } else if (scheduler_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, sigintHandler);
    signal(SIGQUIT, sigQuitHandler);

    do
    {
        printf("group50@SimpleShell $ ");
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL){
            break;
        }
        if (start_sche){
            schedule_the_Processes(readyQueue, runnQueue);
            continue;
        }

        input[strcspn(input, "\n")] = '\0';

        char *raw_input = strdup(input);
        parse_input(input, command, args);

        if (strcmp(command, "stats") == 0){
            display_statistics();
            continue;
        }

        if (strcmp(command, "history") == 0){
            for (int i = 0; i < command_num; i++)
            {
                char *start_time_str = formatTime(command_history[i].start_time.tv_sec);
                char *end_time_str = formatTime(command_history[i].end_time.tv_sec);

                printf("Command: %s\n", command_history[i].command);
                printf("Args: %s\n", command_history[i].args);
                printf("Pid: %i\n", command_history[i].pid);
                printf("Start_Time: %s.%06ld\n", start_time_str, command_history[i].start_time.tv_usec);
                printf("End_Time: %s.%06ld\n", end_time_str, command_history[i].end_time.tv_usec);
                printf("Duration: %f ms\n", command_history[i].duration);
                printf("Waiting Time: %f ms\n", command_history[i].waiting_time);
                printf("\n");

                free(start_time_str);
                free(end_time_str);
            }
            continue;
        }

        if (strcmp(command, "submit") == 0)
        {
            char *line = strdup(raw_input);
            raw_input = raw_input + 7;
            parse_input(raw_input, command, args);
            int priority = 1;
            if (args[1] != NULL){
                priority = atoi(args[1]);
            }
            int stat = launch(command);
            switch (stat){
            case 0:
                execvp(command, args);
                perror("execvp");
                exit(EXIT_FAILURE);
                break;
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                break;
            default:
                kill(stat, SIGSTOP);
                enqueue(stat, priority, readyQueue);
                history com = {0};
                com.command = "submit";
                com.args = strdup(line);
                com.priority = priority;
                com.pid = stat;
                command_history[command_num] = com;
                command_num++;
            }
        }
        else if (strcmp(command, "schedule") == 0){
            schedule_the_Processes(readyQueue, runnQueue);
        }
        else{
            struct timeval start_time, end_time;
            gettimeofday(&start_time, NULL); 

            int status = launch(command);
            waitpid(status, NULL, 0);

            gettimeofday(&end_time, NULL);
            duration = (end_time.tv_sec - start_time.tv_sec) * 1000.0; // Seconds to milliseconds
            duration += (end_time.tv_usec - start_time.tv_usec) / 1000.0; // Microseconds to milliseconds
            if (status > 0) {
                // Parent process
                history com = {0};
                com.command = strdup(command);
                com.args = strdup(raw_input);
                com.pid = status;
                com.start_time = start_time;
                command_history[command_num++] = com;

                // Set up timer for time quantum
                ualarm(TIME_QUANTUM * 1000, 0);

                // Wait for the time quantum to expire
                while (!time_quantum_expired) {
                    pause();
                }

                // Time quantum expired, stop the process and add to ready queue
                kill(status, SIGSTOP);
                enqueue(status, 1, readyQueue); // Default priority 1

                // Signal the scheduler to run
                sem_post(scheduler_sem);

                time_quantum_expired = 0;
            } else if (status == 0) {
                execvp(command, args);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                perror("fork");
            }
            if (status != -1){
                history com;
                com.command = strdup(command);
                com.args = strdup(raw_input);
                com.end_time = end_time;
                com.pid = status;
                com.start_time = start_time;
                com.duration = duration;
                command_history[command_num] = com;
                command_num++;
            }
        }
        // free(raw_input);
        fflush(stdout);
    } while (1);
    printf("Exiting group50@SimpleShell\n\n");

    for (int i = 0; i < command_num; i++)
    {
        char *start_time_str = formatTime(command_history[i].start_time.tv_sec);
        char *end_time_str = formatTime(command_history[i].end_time.tv_sec);

        printf("Command: %s\n", command_history[i].command);
        printf("Args: %s\n", command_history[i].args);
        printf("Pid: %i\n", command_history[i].pid);
        printf("Start_Time: %s.%06ld\n", start_time_str, command_history[i].start_time.tv_usec);
        printf("End_Time: %s.%06ld\n", end_time_str, command_history[i].end_time.tv_usec);
        printf("Duration: %f ms\n", command_history[i].duration);
        printf("Waiting Time: %f ms\n", command_history[i].waiting_time);
        printf("\n");

        free(start_time_str);
        free(end_time_str);
    }
    for (int i = 0; i < command_num; i++)
    {
        free(command_history[i].command);
        free(command_history[i].args);
    }
    return 0;
}