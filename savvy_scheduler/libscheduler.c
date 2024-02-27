/**
 * savvy_scheduler
 * CS 341 - Fall 2023
 */
#include "libpriqueue/libpriqueue.h"
#include "libscheduler.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "print_functions.h"

// global variables
static double total_turnaround_time;
static double total_waiting_time;
static double total_response_time;
static int num_job;

/**
 * The struct to hold the information about a given job
 */
typedef struct _job_info {
    int id;

    /* TODO: Add any other information and bookkeeping you need into this
     * struct. */
    double arrival_time;
    double priority;
    double remaining_time;
    double recent_execution_time;
    double run_time;
    double start_time;
} job_info;

void scheduler_start_up(scheme_t s) {
    switch (s) {
    case FCFS:
        comparision_func = comparer_fcfs;
        break;
    case PRI:
        comparision_func = comparer_pri;
        break;
    case PPRI:
        comparision_func = comparer_ppri;
        break;
    case PSRTF:
        comparision_func = comparer_psrtf;
        break;
    case RR:
        comparision_func = comparer_rr;
        break;
    case SJF:
        comparision_func = comparer_sjf;
        break;
    default:
        printf("Did not recognize scheme\n");
        exit(1);
    }
    priqueue_init(&pqueue, comparision_func);
    pqueue_scheme = s;
    // Put any additional set up code you may need here
    total_response_time = 0;
    total_turnaround_time = 0;
    total_waiting_time = 0;
    num_job = 0;
}

static int break_tie(const void *a, const void *b) {
    return comparer_fcfs(a, b);
}

int comparer_fcfs(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* jobA = ((job*)a)->metadata;
    job_info* jobB = ((job*)b)->metadata;

    if( jobA->arrival_time < jobB->arrival_time )
        return -1;
    else if( jobA->arrival_time > jobB->arrival_time )
        return 1;
    else if( jobA->arrival_time == jobB->arrival_time ){
        return 0;
    }
    return 0;
}

int comparer_ppri(const void *a, const void *b) {
    // Complete as is
    return comparer_pri(a, b);
}

int comparer_pri(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* jobA = ((job*)a)->metadata;
    job_info* jobB = ((job*)b)->metadata;

    if( jobA->priority < jobB->priority )
        return -1;
    else if( jobA->priority > jobB->priority )
        return 1;
    else if( jobA->priority == jobB->priority ){
        return break_tie( a, b );
    }
    return 0;
}

int comparer_psrtf(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* jobA = ((job*)a)->metadata;
    job_info* jobB = ((job*)b)->metadata;

    if( jobA->remaining_time < jobB->remaining_time )
        return -1;
    else if( jobA->remaining_time > jobB->remaining_time )
        return 1;
    else if( jobA->remaining_time == jobB->remaining_time ){
        return break_tie( a, b );
    }
    return 0;
}

int comparer_rr(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* jobA = ((job*)a)->metadata;
    job_info* jobB = ((job*)b)->metadata;

    if( jobA->recent_execution_time < jobB->recent_execution_time )
        return -1;
    else if( jobA->recent_execution_time > jobB->recent_execution_time )
        return 1;
    else if( jobA->recent_execution_time == jobB->recent_execution_time ){
        return break_tie( a, b );
    }
    return 0;
}

int comparer_sjf(const void *a, const void *b) {
    // TODO: Implement me!
    job_info* jobA = ((job*)a)->metadata;
    job_info* jobB = ((job*)b)->metadata;

    if( jobA->remaining_time < jobB->remaining_time )
        return -1;
    else if( jobA->remaining_time > jobB->remaining_time )
        return 1;
    else if( jobA->remaining_time == jobB->remaining_time ){
        return break_tie( a, b );
    }
    return 0;
}

// Do not allocate stack space or initialize ctx. These will be overwritten by
// gtgo
void scheduler_new_job(job *newjob, int job_number, double time,
                       scheduler_info *sched_data) {
    // TODO: Implement me!
    job_info* info = malloc( sizeof(job_info) );
    info->arrival_time = time;
    info->id = job_number;
    info->priority = sched_data->priority;
    info->recent_execution_time = -1;
    info->run_time = sched_data->running_time;
    info->remaining_time = sched_data->running_time;
    info->start_time = -1;

    newjob->metadata = info;
    priqueue_offer( &pqueue, newjob );
}

job *scheduler_quantum_expired(job *job_evicted, double time) {
    // TODO: Implement me!
    if( job_evicted == NULL ){
        return priqueue_peek( &pqueue );
    }

    // set parameters
    job_info* info = job_evicted->metadata;
    if( info->start_time < 0 )
        info->start_time = time - 1;
    info->recent_execution_time = time;
    info->remaining_time -= 1;

    if( pqueue_scheme == PPRI || pqueue_scheme == PSRTF || pqueue_scheme == RR ){
        job* cur = priqueue_poll( &pqueue );
        priqueue_offer( &pqueue, cur );
        return priqueue_peek( &pqueue );
    }
    else{
        return job_evicted;
    }
    return NULL;
}

void scheduler_job_finished(job *job_done, double time) {
    // TODO: Implement me!
    job_info* info = job_done->metadata;

    total_waiting_time += ( time - info->arrival_time - info->run_time );
    total_turnaround_time += ( time - info->arrival_time );
    total_response_time += ( info->start_time - info->arrival_time );
    num_job += 1;

    priqueue_poll( &pqueue );
}

static void print_stats() {
    fprintf(stderr, "turnaround     %f\n", scheduler_average_turnaround_time());
    fprintf(stderr, "total_waiting  %f\n", scheduler_average_waiting_time());
    fprintf(stderr, "total_response %f\n", scheduler_average_response_time());
}

double scheduler_average_waiting_time() {
    // TODO: Implement me!
    return total_waiting_time / (float) num_job;
}

double scheduler_average_turnaround_time() {
    // TODO: Implement me!
    return total_turnaround_time / (float) num_job;
}

double scheduler_average_response_time() {
    // TODO: Implement me!
    return total_response_time / (float) num_job;
}

void scheduler_show_queue() {
    // OPTIONAL: Implement this if you need it!
}

void scheduler_clean_up() {
    priqueue_destroy(&pqueue);
    print_stats();
}
