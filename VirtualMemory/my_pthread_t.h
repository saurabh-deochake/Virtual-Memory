/*
*  my_pthread_t.h
*  my_pthread_t
*
*  Created by Ronil Mehta on 16/02/16.
*  Copyright (c) 2016 Ronil Mehta. All rights reserved.
*  Authors: Ronil Mehta (rvm41),
*           Saurabh Deochake (srd117),
*           Niraj Dholakia (nd387)
*/

#ifndef my_pthread_t_my_pthread_t_h

#define my_pthread_t_my_pthread_t_h

#define MAX_THREAD_COUNT 10
#define MAX_QUEUE_COUNT 3 
#define FIRST_QUEUE_QUANTA 10 //milliseconds
#define THREAD_STACK (1024*1024) //size of the stack
#define THREAD_POOL_SATURATED_RETURN_VALUE 1
#define MALLOC_ERROR 2
#define SCAN_INTERVAL_COUNT 6

#include <ucontext.h>

/* Primitive thread structure which contains all
 * primary information about a thread created using
 * my_pthread_t library function.
 * */

typedef struct {
    int id;
    void* stack;
    ucontext_t context;
    int isFinished;
    int isCleaned;
    long startTimestamp;
    long timeSpent;
    long allocatedQuantum;
} my_pthread_t;

/* A light-weight mutex structure
 * This is a test&set mutex which uses a variable
 * which is modified based on locking-unlocking
 * mechanism.
 * */

typedef struct {
    int isLocked;
} my_pthread_mutex_t;

extern int THREADREQ;
extern my_pthread_t *currentThread;

//Creates a pthread that executes function. Attributes are ignored.
extern int my_pthread_create(my_pthread_t * thread, void * attr, void (*function)(void), void * arg);

//Explicit call to the my_pthread_t scheduler requesting that the current context be swapped out and another be scheduled.
extern void my_pthread_yield();

//Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't NULL, any return value from the thread will be saved.
extern void my_pthread_exit(void *value_ptr);

//Call to the my_pthread_t library ensuring that the calling thread will not  execute until the one it references exits. If value_ptr is not null, the return value of the  exiting thread will be passed back.
int my_pthread_join(my_pthread_t * thread, void **value_ptr);

// Call to my_pthread_t library to initialize the mutex variable
extern int my_pthread_mutex_init(my_pthread_mutex_t *mutex);

//Call to my_pthread_t library to lock the mutex variable
extern int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

//Call to my_pthread_t library to unlock the mutex variable
extern int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

//Call to my_pthread_t library to free up the mutex variable
extern int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

//Call to my_pthread_t library to get a thread to execute 
extern void thread_start(void (*t_func)(void));

extern void init_threads();

//Call to my_pthread_t library to copy attributes of a thread to others
extern void deepCopyThreads(my_pthread_t *t1,my_pthread_t *t2);

extern void scanSchedulerQueues();

#endif
