/*
 * student.c
 * Multithreaded OS Simulation for CS 2200
 * Fall 2024
 *
 * This file contains the CPU scheduler for the simulation.
 */

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "student.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/** Function prototypes **/
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);

static unsigned int cpu_count;
static int preemption_time;

/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 *
 * rq is a pointer to a struct you should use for your ready queue
 * implementation. The head of the queue corresponds to the process
 * that is about to be scheduled onto the CPU, and the tail is for
 * convenience in the enqueue function. See student.h for the
 * relevant function and struct declarations.
 *
 * Similar to current[], rq is accessed by multiple threads,
 * so you will need to use a mutex to protect it. ready_mutex has been
 * provided for that purpose.
 *
 * The condition variable queue_not_empty has been provided for you
 * to use in conditional waits and signals.
 *
 * Please look up documentation on how to properly use pthread_mutex_t
 * and pthread_cond_t.
 *
 * A scheduler_algorithm variable and sched_algorithm_t enum have also been
 * supplied to you to keep track of your scheduler's current scheduling
 * algorithm. You should update this variable according to the program's
 * command-line arguments. Read student.h for the definitions of this type.
 */
static pcb_t **current;
static queue_t *rq;

static pthread_mutex_t current_mutex;
static pthread_mutex_t queue_mutex;
static pthread_cond_t queue_not_empty;

static sched_algorithm_t scheduler_algorithm;
static unsigned int cpu_count;
static unsigned int age_weight;

/** ------------------------Problem 3-----------------------------------
 * Checkout PDF Section 5 for this problem
 * 
 * priority_with_age() is a helper function to calculate the priority of a process
 * taking into consideration the age of the process.
 * 
 * It is determined by the formula:
 * Priority With Age = Priority - (Current Time - Enqueue Time) * Age Weight
 * 
 * @param current_time current time of the simulation
 * @param process process that we need to calculate the priority with age
 * 
 */
extern double priority_with_age(unsigned int current_time, pcb_t *process) {
    /* FIX ME */
    return process->priority - (current_time - process->enqueue_time) * age_weight;
}

/** ------------------------Problem 0 & 3-----------------------------------
 * Checkout PDF Section 2 and 5 for this problem
 * 
 * enqueue() is a helper function to add a process to the ready queue.
 * 
 * NOTE: For Priority, FCFS, and SRTF scheduling, you will need to have additional logic
 * in this function and/or the dequeue function to account for enqueue time
 * and age to pick the process with the smallest age priority.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 *
 * @param queue pointer to the ready queue
 * @param process process that we need to put in the ready queue
 */
void enqueue(queue_t *queue, pcb_t *process)
{
    /* FIX ME */
    // whatever calls this will have a lock, so no need to give this function one
    
    // two functions (right now) call enqueue: wake_up() and preempt()
    // wake_up() and preempt() sets process->state = PROCESS_READY

    process->enqueue_time = get_current_time();
    process->next = NULL;

    if (is_empty(queue)) { // initialize queue with the pcb. regardless of the algorithm, we add 1 process to a queue of size 0 with no other process to compare to
        queue->head = process;
        queue->tail = process;
    } else {
        // just add to tail
        queue->tail->next = process;
        queue->tail = process;
    }
    
    pthread_cond_signal(&queue_not_empty);
}

/**
 * dequeue() is a helper function to remove a process to the ready queue.
 *
 * NOTE: For Priority, FCFS, and SRTF scheduling, you will need to have additional logic
 * in this function and/or the enqueue function to account for enqueue time
 * and age to pick the process with the smallest age priority.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 * 
 * @param queue pointer to the ready queue
 */
pcb_t *dequeue(queue_t *queue)
{
    /* FIX ME */
    // NOTE: queue.size() should never be less than 1 when this function is called
    pcb_t *curr = queue->head; // Start at the head
    pcb_t *curr_prev = NULL;
    pcb_t *best = queue->head; // Initialize space for selected process
    pcb_t *best_prev = NULL;

    // the way we remove a pcb from the rq differs per scheduling algorithm...
    switch(scheduler_algorithm) {
        case FCFS:
            while(curr != NULL) {
                if (curr->arrival_time < best->arrival_time) { // if arrival time of curr pcb is less than what we have stored..
                    best = curr; // choose new pcb
                    best_prev = curr_prev;
                }
                curr_prev = curr;
                curr = curr->next; // if not, point to the next pcb in the ready queue
            }
            break;
        case RR:
            // RR is just FCFS in terms of dequeue logic, since preemption is handled separately
            break;
        case PA:
            while(curr != NULL) {
                if (priority_with_age(get_current_time(), curr) < priority_with_age(get_current_time(), best)) { // if arrival time of curr pcb is less than what we have stored..
                    best = curr; // choose new pcb
                    best_prev = curr_prev;
                }
                curr_prev = curr;
                curr = curr->next; // if not, point to the next pcb in the ready queue
            }
            break;
        case SRTF:
            while(curr != NULL) {
                if (curr->total_time_remaining < best->total_time_remaining) { // if arrival time of curr pcb is less than what we have stored..
                    best = curr; // choose new pcb
                    best_prev = curr_prev;
                }
                curr_prev = curr;
                curr = curr->next; // if not, point to the next pcb in the ready queue
            }
            break;
    }

    if (best == queue->head && best == queue->tail) { // queue has one PCB
        queue->head = NULL;
        queue->tail = NULL;
    } else if (best == queue->head) { // dequeue happens at head
        queue->head = best->next;
    } else { // dequeue happens anywhere else
        best_prev->next = best->next;
        if (best == queue->tail) { // if PCB being removed occurs at the tail..
            queue->tail = best_prev; // set the queue's tail to PCB before what's being removed
        }
    }

    best->next = NULL;
    return best;
}

/** ------------------------Problem 0-----------------------------------
 * Checkout PDF Section 2 for this problem
 * 
 * is_empty() is a helper function that returns whether the ready queue
 * has any processes in it.
 * 
 * @param queue pointer to the ready queue
 * 
 * @return a boolean value that indicates whether the queue is empty or not
 */
bool is_empty(queue_t *queue)
{
    /* FIX ME */
    if (!queue) {
        fprintf(stderr, "Queue not initialized.\n");
        return 1;
    }
    
    return (queue->head == NULL);
}

/** ------------------------Problem 1B-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * schedule() is your CPU scheduler.
 * 
 * Remember to specify the timeslice if the scheduling algorithm is Round-Robin
 * 
 * @param cpu_id the target cpu we decide to put our process in
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    
    if (is_empty(rq)) {
        context_switch(cpu_id, NULL, -1); // context switch to the idle process with FCFS (-1) preemption time (infinite)
        pthread_mutex_unlock(&current_mutex);
        return;
    }
    pcb_t *next_process = dequeue(rq);
    next_process->state = PROCESS_RUNNING;
    printf("Process: %s | Priority: %d | Arrival Time: %d | Current Time: %d | Enqueue Time: %d\n", next_process->name, next_process->priority, next_process->arrival_time, get_current_time(), next_process->enqueue_time);
    
    switch(scheduler_algorithm) {
        case RR:
            context_switch(cpu_id, next_process, preemption_time);
            break;
        case FCFS:
        case PA:
        case SRTF:
            context_switch(cpu_id, next_process, -1);
            break;  
    }

    current[cpu_id] = next_process; // throw 'next_process' on CPU 'cpu_id' since it's now running

    pthread_mutex_unlock(&current_mutex);
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled. This function should block until a process is added
 * to your ready queue.
 *
 * @param cpu_id the cpu that is waiting for process to come in
 */
extern void idle(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);

    while(is_empty(rq)) {
        if(current[cpu_id] != NULL) {
            current[cpu_id]->state = PROCESS_WAITING;
        }
        pthread_cond_wait(&queue_not_empty, &current_mutex);
    }
    
    pthread_mutex_unlock(&current_mutex);

    schedule(cpu_id);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
}

/** ------------------------Problem 2 & 3-----------------------------------
 * Checkout Section 4 and 5 for this problem
 * 
 * preempt() is the handler used in Round-robin, Preemptive Priority, and SRTF scheduling.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 * 
 * @param cpu_id the cpu in which we want to preempt process
 */
extern void preempt(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);

    pcb_t* curr_process = current[cpu_id]; // Get current process
    curr_process->state = PROCESS_READY; // Set its state to ready
    enqueue(rq, curr_process); // Placed back in RQ

    pthread_mutex_unlock(&current_mutex);

    schedule(cpu_id); // Select a new runnable process to be placed in **current[]
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3 for this problem
 * 
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * @param cpu_id the cpu that is yielded by the process
 */
extern void yield(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t *yielding_process = current[cpu_id]; // get pointer to cpu_id's pcb
    if (yielding_process != NULL) {
        yielding_process->state = PROCESS_WAITING; // set the state to waiting
    }
    pthread_mutex_unlock(&current_mutex);

    // printf("[yield] CPU %u yielded process %d\n", cpu_id, yielding_process->pid);

    schedule(cpu_id); // call schedule() to context switch
}

/**  ------------------------Problem 1A-----------------------------------
 * Checkout PDF Section 3
 * 
 * terminate() is the handler called by the simulator when a process completes.
 * 
 * @param cpu_id the cpu we want to terminate
 */
extern void terminate(unsigned int cpu_id)
{
    /* FIX ME */
    pthread_mutex_lock(&current_mutex);
    pcb_t *terminating_process = current[cpu_id];
    if(terminating_process != NULL) {
        terminating_process->state = PROCESS_TERMINATED;
    }
    pthread_mutex_unlock(&current_mutex);

    schedule(cpu_id);
}

/**  ------------------------Problem 1A & 3---------------------------------
 * Checkout PDF Section 3 and 5 for this problem
 * 
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes. 
 * This method will also need to handle priority and SRTF preemption.
 * Look in section 5 of the PDF for more info on priority.
 * Look in section 6 of the PDF for more info on SRTF preemption.
 * 
 * We have provided a function get_current_time() which is prototyped in os-sim.h.
 * Look there for more information.
 * 
 * @param process the process that finishes I/O and is ready to run on CPU
 */
extern void wake_up(pcb_t *process)
{
    /* FIX ME */
    // however wake_up() is the very first function called when a process is created, so it should handle process->arrival_time() 

    if (process->state == PROCESS_NEW) {
        process->arrival_time = get_current_time(); // set its arrival time
    } 

    process->state = PROCESS_READY;
    
    pthread_mutex_lock(&queue_mutex);
    enqueue(rq, process); // take the process on the I/O queue and put it in the Ready queue
    pthread_mutex_unlock(&queue_mutex);
}

/**
 * main() simply parses command line arguments, then calls start_simulator().
 * Add support for -r, -p, and -s parameters. If no argument has been supplied, 
 * you should default to FCFS.
 * 
 * HINT:
 * Use the scheduler_algorithm variable (see student.h) in your scheduler to 
 * keep track of the scheduling algorithm you're using.
 */
int main(int argc, char *argv[])
{
    /* FIX ME */
    age_weight = 0;
    int opt;
    scheduler_algorithm = FCFS;

    if (argc < 2)
    {
        fprintf(stderr, "CS 2200 Project 4 -- Multithreaded OS Simulator\n"
                        "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p <age weight> | -s ]\n"
                        "    Default : FCFS Scheduler\n"
                        "         -r : Round-Robin Scheduler\n1\n"
                        "         -p : Priority Aging Scheduler\n"
                        "         -s : Shortest Remaining Time First\n");
        return -1;
    }

    /* Parse the command line arguments */
    char *endptr;
    cpu_count = strtoul(argv[1], &endptr, 10);
    if (*endptr != '\0' || cpu_count == 0) {
        fprintf(stderr, "Invalid CPU count: %s\n", argv[1]);
        return -1;
    }

    optind = 2;
    while ((opt = getopt(argc, argv, "r:p:s")) != -1) { // Round-Robin + Preemptive Scheduling require args, SRTF doesn't
        switch (opt) {
            case 'r':
                preemption_time = (int)atoi(optarg);
                scheduler_algorithm = RR;
                printf("Scheduling algorithm: RR\n");
                break;
            case 'p':
                age_weight = (unsigned int)atoi(optarg);
                scheduler_algorithm = PA;
                printf("Scheduling algorithm: PA\n");
                break;
            case 's':
                scheduler_algorithm = SRTF;
                printf("Scheduling algorithm: SRTF\n");
                break;
            default:
                scheduler_algorithm = FCFS;
                printf("Scheduling algorithm: FCFS\n");
                break;
        }
    }

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t *) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_cond_init(&queue_not_empty, NULL);
    rq = (queue_t *)malloc(sizeof(queue_t));
    assert(rq != NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;
}

#pragma GCC diagnostic pop
