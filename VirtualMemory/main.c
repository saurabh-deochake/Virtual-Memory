//
//  main.c
//  VirtualMemory
//
//  Created by Ronil Mehta(RVM41) on 29/03/16.
//  Copyright (c) 2016 Ronil Mehta. All rights reserved.
//

#include <stdio.h>
#include "my_malloc.h"
#include "my_pthread_t.h"
#include <unistd.h>
#include <sys/time.h>

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

my_pthread_mutex_t mutex;

void threadfunc1(){
    //my_pthread_mutex_lock(&mutex);
    int i;
    
    int numb = 6194304;
    char* test = malloc(6194304);
    *(test) = 'd';
    *(test+1) = 'e';
    *(test+2) = 'f';
    *(test+4000000) = 'm';
    
    for(i = 0; i < 8 ; i++){
        sleep(1);
        printf("Thread1: test is: %c\n",*(test+i%3));
        printf("Thread1: test is: %c\n",*(test+4000000));
    //my_pthread_mutex_unlock(&mutex);
    }
    free(test);
}
/* Name:     threadfunc2
 * Input:    None
 * Output:   None returned
 * Function: Print a statement for certain amount of times
 **/
void threadfunc2(){
    //my_pthread_mutex_lock(&mutex);
    int i;
    
    int numb = 4194304;
    char* test = malloc(4194304);
    *(test) = 'a';
    *(test+1) = 'b';
    *(test+2) = 'c';
    *(test+4000000) = 'n';
    //free(yaw);
    for(i = 0; i < 6 ; i++){
        sleep(1);
        printf("Thread2: test is: %c\n",*(test+i%3));
        printf("Thread2: test is: %c\n",*(test+4000000));
    }
    free(test);
    //my_pthread_mutex_unlock(&mutex);
}

/* Name:     threadfunc3
 * Input:    None
 * Output:   None returned
 * Function: Print a statement for certain amount of times
 **/
void threadfunc3(){
    my_pthread_mutex_lock(&mutex);
    int i;
    
    for(i = 0; i < 2 ; i++){
        sleep(1);
        printf("Thread3\n");
        
    }
    for(i = 0; i < 4 ; i++){
        my_pthread_yield();
        sleep(0.1);
        printf("POST YIELD: ThreadFunc3\n");
    }
    my_pthread_mutex_unlock(&mutex);
}



int main(int argc, const char * argv[]) {
    
    printf("Start!\n");
    
    my_pthread_t thread1,thread2,thread3;
    init_threads();
    struct timeval end, start;
    gettimeofday(&start, NULL);
    
    /* Initialize the mutex variable.
     * This will reset the in-built count
     * variable to 0 making it available to threads.
     **/
    my_pthread_mutex_init(&mutex);
    
    //Create threads
    my_pthread_create(&thread1, NULL, &threadfunc1,NULL);
    my_pthread_create(&thread2, NULL, &threadfunc2,NULL);
    my_pthread_create(&thread3, NULL, &threadfunc3,NULL);
    
    //Call join on the threads one by one
    my_pthread_join(&thread2,NULL);
    my_pthread_join(&thread1,NULL);
    my_pthread_join(&thread3,NULL);
    
    /* Destroying the mutex variable.
     * Before destroying the mutex variable, we make sure
     * that we unlock it by internally calling the unlock.
     **/
    my_pthread_mutex_destroy(&mutex);
    gettimeofday(&end, NULL);
    //printf("Execution time %d\n", end.tv_usec - start.tv_usec);
    printf("Ending main!\n");
    
    shutDown();
    
    return 0;
}
