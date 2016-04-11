/*
*  my_queue.c
*  my_pthread_t
*
*  All the primitive scheduler queue related structures
*
*  Created by Ronil Mehta on 22/02/16.
*  Copyright (c) 2016 Ronil Mehta. All rights reserved.
*  Authors: Ronil Mehta (rvm41),
*           Saurabh Deochake (srd117),
*           Niraj Dholakia (nd387)
*/

#ifndef my_pthread_t_my_queue_h
#define my_pthread_t_my_queue_h

#include "my_pthread_t.h"

// Basic structure for queue
struct Node {
    my_pthread_t *thread;
    struct Node *next;
};

struct Queue {
    struct Node *head;
    struct Node *tail;
};

// Call to my_queue and my_pthread_t library to add element to the queue
int addElementToQueue(my_pthread_t *thread,struct Queue *queue);

// Call to my_queue and my_pthread_t library to remove element to the queue
int removeElementFromQueue(struct Queue *queue,my_pthread_t **thread);


/* 
 
 //Tutorial by Ronil Mehta on how to use the my_queue.h library

 queue1 = malloc(sizeof(struct Queue));
 my_pthread_t *testT = malloc(sizeof(my_pthread_t));
 testT->id = 111;
 my_pthread_t *testT2 = malloc(sizeof(my_pthread_t));
 testT2->id = 222;
 my_pthread_t *testT3 = malloc(sizeof(my_pthread_t));
 testT3->id = 333;
 addElementToQueue(testT,queue1);
 addElementToQueue(testT2, queue1);
 addElementToQueue(testT3, queue1);
 my_pthread_t* temp;
 removeElementFromQueue(queue1,&temp);
 printf("%d\n",temp->id);
 removeElementFromQueue(queue1, &temp);
 printf("%d\n",temp->id);
 removeElementFromQueue(queue1, &temp);
 printf("%d\n",temp->id);
 removeElementFromQueue(queue1, &temp);
 
 */


#endif
