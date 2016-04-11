/*
*  my_pthread_t.c
*  my_pthread_t
*
*  All the primitive functionalities of my_pthread_t
*  library go here.
*
*  Created by Ronil Mehta on 18/02/16.
*  Copyright (c) 2016 Ronil Mehta. All rights reserved.
*  Authors: Ronil Mehta (rvm41),
*           Saurabh Deochake (srd117),
*           Niraj Dholakia (nd387)
*/

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <stdio.h>
#include <inttypes.h>
#include "my_pthread_t.h"
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "my_queue.h"
#include "my_malloc.h"

//struct Queue *queue1;

struct Queue queue[MAX_QUEUE_COUNT];
static int currentQueue = 0;
static my_pthread_t *currentThread = 0;
static int inMainThread = 1;
static ucontext_t main_context;
static int id = 1;
static int cycle_counter = 1;
static int firstFlag = 0;
static int endThread = 0;
int THREADREQ = 0;

void changeContext(int signum);
void scheduler();
long getCurrentTimestamp();
void setAlarm(int seconds,suseconds_t microseconds);


void init_threads()
{
    //queue1 = malloc(sizeof(struct Queue));
    /*int i;
     for ( i = 0; i < MAX_QUEUE_COUNT; ++ i )
     {
        
     }
    
    return;*/
}

/* Name:     thread_start
 * Input:    Function to execute
 * Output:   None
 * Function: Call the function to execute, when done,
 *           Update the state of the thread to finished
 *           and give up the scheduler to others.
 **/
void thread_start(void (*t_func)(void)){
    //printf("Entering a thread\n");
    THREADREQ = currentThread->id;
    mprotectFunc();
    t_func();
    currentThread->isFinished = 1;
    my_pthread_yield();
}

/* Name:     my_pthread_yield
 * Input:    None
 * Output:   None
 * Function: Check the context of the thread, switch it
 *           appropriately to check the scheduler queue
 *           and remove first thread and give scheduler to
 *           the next in the queue.
 **/
void my_pthread_yield(){
    //printf("Yeilding...\n");
    THREADREQ = 0;
    mprotectFunc();
    
    long currentTimestamp = getCurrentTimestamp();
    setAlarm(0,0);
    //alarm(0);
    if (inMainThread == 0) {
        printf("Before main swapContext*************\n");

        currentThread->timeSpent = currentThread->timeSpent + currentTimestamp - currentThread->startTimestamp;
        
        inMainThread = 1; //In main thread
        // After we swap the context with main context,
        // we leave the main thread context
        swapcontext(&currentThread->context, &main_context );
        inMainThread = 0; //Outside main context
        
        return;
    }
    else{
        
        cycle_counter++;
        cycle_counter = (cycle_counter%SCAN_INTERVAL_COUNT);
        if(cycle_counter == 0){
            scanSchedulerQueues();
        }
        
        scheduler();
        
    }
}

/* Name:     scheduler
 * Input:    None
 * Output:   None returned
 * Function: Primitive Scheduler functions to move the threads
 *           on the queue, check the queue, allocate quanta
 *           on specific alarms change the context of threads.
 **/
void scheduler(){
    
    
    if((currentThread!= 0 && currentThread->isFinished == 1) || endThread == 1){
        endThread = 0;
        //printf("Cleaning thread with id %d\n",currentThread->id);
        //Clean up the threads
        my_pthread_t *temp;
        removeElementFromQueue(&queue[currentQueue],&temp);
        free(temp->stack);
        temp->isCleaned = 1;
    }
    else{
        if(firstFlag == 1){
            //printf("Round robin Sched!\n");
            my_pthread_t *temp;
            removeElementFromQueue(&queue[currentQueue],&temp);
        
            int nextQueue = (currentQueue + 1)%MAX_QUEUE_COUNT;
            if (temp->timeSpent < FIRST_QUEUE_QUANTA) {
                //printf("Shifting to thread%d to same queue\n",temp->id);
                nextQueue = currentQueue;
            }
            else{
                currentThread->timeSpent = 0;
            }
        
            if(nextQueue == 0){
                addElementToQueue(temp, &queue[currentQueue]);
            }
            else{
                addElementToQueue(temp, &queue[nextQueue]);
            }
        }
        else{
            firstFlag = 1;
        }
                                                                          
    }
    if(queue[currentQueue].head == 0){
        int tempCurrentQueue = currentQueue;
        currentQueue = (currentQueue+1)%MAX_QUEUE_COUNT;
        while(queue[currentQueue].head == 0 && currentQueue != tempCurrentQueue){
            currentQueue = (currentQueue+1)%MAX_QUEUE_COUNT;
        }
        if(currentQueue == tempCurrentQueue){
            //printf("Queues are empty\n");
            return;
        }
        //currentThread = 0;
        //return;
    }
    currentThread = queue[currentQueue].head->thread;
    

    long quantumAllocation = FIRST_QUEUE_QUANTA - currentThread->timeSpent;
    if (quantumAllocation <= 0) {
        //printf("QA is 1000\n");
        quantumAllocation = 1000;
    }
    //printf("Q.A. for thread%d is %ld\n bcause timeSpnt is %ld\n",currentThread->id,quantumAllocation,currentThread->timeSpent);
    inMainThread = 0;
    //THREADREQ = 0;
    //mprotectFunc();
    signal(SIGALRM, changeContext); //Change context on SIGALRM
    currentThread->startTimestamp = getCurrentTimestamp();
    //alarm(quantumAllocation);
    setAlarm((int)(quantumAllocation/1000),quantumAllocation%1000);
    printf("Before thread swapContext %p   %p\n",&main_context,&currentThread->context);
    swapcontext(&main_context,&currentThread->context);
    printf("After thread swapContext\n");
    inMainThread = 1; //Come back to main thread
    //THREADREQ = 0;
    //mprotectFunc();
}

/* Name:     changeContext
 * Input:    Signal number
 * Output:   None returned
 * Function: Swap the context with main thread
 **/
void changeContext(int signum){
    currentThread->timeSpent = FIRST_QUEUE_QUANTA;
    swapcontext(&currentThread->context,&main_context);
}


/* Name:     my_pthread_create
 * Input:    pointer to thread structure and attributes values
 * Output:   0 returned on success, Custom error if failure
 * Function: Performing the important thread creation functions.
 *           Create a context for created thread and return on success.
 **/
int my_pthread_create(my_pthread_t * thread, void * attr, void (*function)(void), void * arg){
    //printf("start creation!\n");
    //if(nextQueueIndex == MAX_THREAD_COUNT) return THREAD_POOL_SATURATED_RETURN_VALUE;
    // Set values to some thread state related stuff
    THREADREQ = 0;
    mprotectFunc();
    
    currentQueue = 0;

    thread->isFinished = 0;
    thread->isCleaned = 0;
    thread->startTimestamp = 0;
    thread->timeSpent = 0;
    getcontext(&(thread->context)); //fetch the context
    thread->context.uc_link = 0;
    printf("Calling Malloc!!!!\n");
    thread->stack = malloc( THREAD_STACK );
    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = THREAD_STACK;
    thread->context.uc_stack.ss_flags = 0;
    
    //Check if we can allocate memory for stack
    if ( thread->stack == 0 )
    {
        printf( "Error: Could not allocate stack.\n" );
        return MALLOC_ERROR;
    }
    thread->id = id;
    id++;
    addElementToQueue(thread, &queue[currentQueue]); //add to scheduler
    // Create a context for newly created thread                                                    
    makecontext( &thread->context, (void (*)(void)) &thread_start, 1, function );
    return 0;
}



/* Name:     my_pthread_join
 * Input:    pointer to thread structure
 * Output:   0 if already cleaned, None otherwise
 * Function: Check if thread is already cleaned, if
 *           yes then return 0, otherwise yeild to other
 *           thread.
 **/
int my_pthread_join(my_pthread_t * thread, void **value_ptr){
    while (1) {
        if(thread->isCleaned == 1){
            //printf("Ending threadid %d\n",thread->id);
            return 0;
        }
        else{
            my_pthread_yield();
        }
    
    }
}


/* Name:     deepCopyThreads
 * Input:    Pointers to two thread structs
 * Output:   None returned
 * Function: Copy all the thread attributes of one
 *           thread into another thread.
 **/
void deepCopyThreads(my_pthread_t *t1,my_pthread_t *t2){

    t1->id = t2->id;
    t1->context = t2->context;
    t1->isFinished = t2->isFinished;
    t1->stack = t2->stack;
    
}

/* Name:     scanSchedulerQueues
 * Input:    None
 * Output:   None returned
 * Function: Traverse through the scheduler queue to
 *           place an element at the back of the queue.
 **/
void scanSchedulerQueues(){
    //printf("Move all jobs to Topmost queue\n");
    currentQueue = 0;
    struct Queue *primary = &queue[0];
    int i;
    for(i = 1; i < MAX_QUEUE_COUNT ; i++){
        while(queue[i].head != 0){
            my_pthread_t *temp;
            removeElementFromQueue(&queue[i], &temp);
            addElementToQueue(temp, &queue[0]);
        }
    }

}

/* Name:     getCurrentTimestamp
 * Input:    None
 * Output:   Current timestamp in the system
 * Function: Calculate current timestamp and return it
 **/
long getCurrentTimestamp(){

    struct timespec spec;
#ifdef __MACH__ // OS X does not have clock_gettime, using clock_get_time
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    spec.tv_sec = mts.tv_sec;
    spec.tv_nsec = mts.tv_nsec;
#else
    clock_gettime(CLOCK_REALTIME, &spec);
#endif
    
    return spec.tv_nsec;


}

/* Name:     setAlarm
 * Input:    Seconds and microseconds values
 * Output:   None returned, set timer for specified value
 * Function: Set a timer for specified value passed to function
 **/
void setAlarm(int seconds,suseconds_t microseconds){
    
    
    struct itimerval tout_val;
    
    //if(seconds == 0 && microseconds == 0){
        //tout_val.it_value = 0;
    //}else{
        tout_val.it_interval.tv_sec = 0;
        tout_val.it_interval.tv_usec = 0;
        tout_val.it_value.tv_sec = seconds; /* set timer for "INTERVAL (10) seconds */
        tout_val.it_value.tv_usec = microseconds;
    //}
    setitimer(ITIMER_REAL, &tout_val,0);
}

/* Name: my_pthread_mutex_init
 * Input:    mutex structure pointer
 * Output:   return on success
 * Function: Initialize the "mutex" structure and allocate
 *           the memory. Set the lock variable to 0.
 **/
int my_pthread_mutex_init(my_pthread_mutex_t *mutex){
    int mutexSize = sizeof(my_pthread_mutex_t);
    mutex = (my_pthread_mutex_t *) malloc(mutexSize);
    mutex -> isLocked = 0;
    return(EXIT_SUCCESS);
}

/* Name: my_pthread_mutex_lock
 * Input:    mutex structure pointer
 * Output:   return on success
 * Function: If mutex is not locked, set the
 *           lock variable to 0. If not, call
 *           yeild function until mutex is available.
 **/
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex){
    if (mutex == NULL){
        my_pthread_mutex_init(mutex);
    }
    
    //check if the mutex is already locked
    while(1){
        if (mutex -> isLocked){
        my_pthread_yield();
        }
    //when it is not, set lock variable to 1
        else{
            mutex -> isLocked = 1;
            return(EXIT_SUCCESS);
        }
    }
}

/* Name:     my_pthread_mutex_unlock
 * Input:    mutex structure pointer
 * Output:   return on success
 * Function: If mutex is locked, set the lock
 *           value to 0. Return on success.  
 **/

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){  
    if (mutex == NULL){
        my_pthread_mutex_init(mutex);
    }
    
    // If mutex is locked, set the lock variable to 0.
    if (mutex-> isLocked){
        mutex-> isLocked = 0;    
    }
    return(EXIT_SUCCESS);
}

/* Name:     my_pthread_mutex_destroy
 * Input:    mutex structure pointer
 * Output:   return on success
 * Function: Free up the mutex variable. 
 *           **WARNING**
 *           Before you destroy the mutex varible,
 *           make sure you unlock it.
 **/

int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){
    // Unlock the mutex variable before freeing up.
    my_pthread_mutex_unlock(mutex);
    mutex = NULL;
    
    return(EXIT_SUCCESS);
}



void my_pthread_exit(void *value_ptr){
    endThread = 1;
    my_pthread_yield();

}

