/*
*  my_queue.c
*  my_pthread_t
*
*  All the primitive scheduler queue related functions
*
*  Created by Ronil Mehta on 22/02/16.
*  Copyright (c) 2016 Ronil Mehta. All rights reserved.
*  Authors: Ronil Mehta (rvm41),
*           Saurabh Deochake (srd117),
*           Niraj Dholakia (nd387)
*/

#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif


#include <stdio.h>
#include <stdlib.h>
#include "my_queue.h"
#include "my_malloc.h"


/* Name:     addElementToQueue
 * Input:    Pointer to thread structure and scheduler queue pointer
 * Output:   1 returned on success
 * Function: Allocate the memory for new element in the queue and
 *           add it to the queue used by scheduler
 **/
int addElementToQueue(my_pthread_t *thread,struct Queue *queue){
    
    if(queue->head == 0){
        //printf("h1\n");
        queue->head = malloc(sizeof(struct Node));
        queue->head->thread = thread;
        queue->tail = queue->head;
        return 1;
    }
    else{
        //printf("h2\n");
        queue->tail->next = malloc(sizeof(struct Node));
        queue->tail = queue->tail->next;
        
        queue->tail->thread = thread;
    }
    
    
    return 1;
}

/* Name:     removeElementFromQueue
 * Input:    Element of thread structure and scheduler queue pointer
 * Output:   1 returned on success, 0 otherwise
 * Function: Remove the specified element from the queue and return
 **/
int removeElementFromQueue(struct Queue *queue,my_pthread_t **thread){
    
    if(queue->head == 0){
        return 0;
    }
    
    if(queue->head == queue->tail){
        //deepCopyThreads(thread,queue->head->thread);
        //printf("h3\n");
        *thread = queue->head->thread;
        free(queue->head);
        queue->head = 0;
        queue->tail = 0;
        //return thread;
    }
    else{
        //printf("h4\n");
        *thread = queue->head->thread;
        struct Node *temp;
        temp = queue->head;
        queue->head = queue->head->next;
        //printf("btemp:%x\n",*thread);
        free(temp);
        //printf("atemp:%x\n",*thread);
    }
    
    return 1;
}