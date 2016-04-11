//
//  main.c
//  VirtualMemory
//
//  Created by Ronil Mehta on 29/03/16.
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
    for(i = 0; i < 8 ; i++){
        sleep(1);
        printf("ThreadFunc1\n");}
    //my_pthread_mutex_unlock(&mutex);
}

/* Name:     threadfunc2
 * Input:    None
 * Output:   None returned
 * Function: Print a statement for certain amount of times
 **/
void threadfunc2(){
    //my_pthread_mutex_lock(&mutex);
    int i;
    
    char* test = malloc(4);//myallocate(4, "test ", "test" ,THREADREQ);
    *(test) = 'a';
    *(test+1) = 'b';
    *(test+2) = 'c';
    printf("test is:%p %c%c%c\n",test,*(test),*(test+1),*(test+2));
    free(test);//mydeallocate(test, "test", "test", GthreadID);
    
    //int numb = sizeof(char);
    //char* yaw = (char*)malloc(numb);
    //*yaw = "5";
    //free(yaw);
    for(i = 0; i < 6 ; i++){
        sleep(1);
        printf("ThreadFunc2\n");
    }
    //my_pthread_mutex_unlock(&mutex);
}

/* Name:     threadfunc3
 * Input:    None
 * Output:   None returned
 * Function: Print a statement for certain amount of times
 **/
void threadfunc3(){
    //my_pthread_mutex_lock(&mutex);
    int i;
    
    for(i = 0; i < 2 ; i++){
        sleep(1);
        printf("ThreadFunc3\n");
        
    }
    for(i = 0; i < 4 ; i++){
        my_pthread_yield();
        sleep(0.1);
        printf("POST YIELD: ThreadFunc3\n");
        //my_pthread_exit(NULL);
    }
    //my_pthread_mutex_unlock(&mutex);
}



int main(int argc, const char * argv[]) {
    
    printf("Start!");
    
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
    printf("Execution time %d\n", end.tv_usec - start.tv_usec);
    printf("Ending main!\n");
    
    
    //initMemoryStructures();
    //mprotectFunc();
    //GthreadID=THREADREQ;
    /*char* test = malloc(4);//myallocate(4, "test ", "test" ,THREADREQ);
    *(test) = 'a';
    *(test+1) = 'b';
    *(test+2) = 'c';
    printf("test is:%p %c%c%c\n",test,*(test),*(test+1),*(test+2));
    free(test);//mydeallocate(test, "test", "test", GthreadID);
    //GthreadID=THREADREQ;
    char* test1 = malloc(4);//myallocate(4, "test ", "test" , THREADREQ);
    mprotectFunc();
    *(test1) = 'd';
    *(test1+1) = 'e';
    *(test1+2) = 'f';
    printf("test is:%p %c%c%c\n",test1,*(test1),*(test1+1),*(test1+2));
     */
    
    /*test = myallocate(4, "test ", "test" , 1);
    //printf("test is:%p\n",test);
    *(test) = 'd';
    *(test+1) = 'e';
    *(test+2) = 'f';
    //printf("Line b\n");
    printf("test is:%p %c%c%c\n",test,*(test),*(test+1),*(test+2));*/
    
    /*char* test2 = myallocate(3, "test ", "test" , 2);
    mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    GthreadID = 2;
    *(test2) = 't';
    *(test2+1) = 'u';
    *(test2+2) = 'v';
    //printf("Line b\n");
    
    mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    GthreadID = 2;
    printf("test is:%p %c%c%c\n",test2,*(test2),*(test2+1),*(test2+2));*/
    //char* pm = getPhyMem();
    //int temp = getByteAdditionsForNthPage(0);
    //printf("%c%c%c\n",*(pm+temp),*(pm+temp+1),*(pm+temp+2));
    //printf("test is %p, and pm+temp is %p\n",test,pm+temp);
    
    return 0;
}
