//
//  main.c
//  VirtualMemory
//
//  Created by Ronil Mehta on 29/03/16.
//  Copyright (c) 2016 Ronil Mehta. All rights reserved.
//

#include <stdio.h>
#include "my_malloc.h"
#include <sys/mman.h>


int main(int argc, const char * argv[]) {

    initMemoryStructures();
    mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    char* test = myallocate(4099, "test ", "test" , 1);
    //printf("Line a\n");
    GthreadID=1;
    *(test) = 'a';
    *(test+1) = 'b';
    *(test+2) = 'c';
    *(test+4098) = 'r';
    //printf("Line b\n");
    printf("test is:%p %c%c%c\n",test,*(test),*(test+1),*(test+4098));
    
    mydeallocate(test, "test", "test", 1);
    //mydeallocate(test, "test", "test", 1);
    char* test1 = myallocate(4093, "test ", "test" , 1);
    GthreadID=1;
    mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    //printf("test is:%p\n",test);
    *(test1) = 'd';
    *(test1+1) = 'e';
    *(test1+2) = 'f';
    //printf("Line b\n");
    printf("test is:%p %c%c%c\n",test1,*(test1),*(test1+1),*(test1+2));
    
    /*test = myallocate(4, "test ", "test" , 1);
    //printf("test is:%p\n",test);
    *(test) = 'd';
    *(test+1) = 'e';
    *(test+2) = 'f';
    //printf("Line b\n");
    printf("test is:%p %c%c%c\n",test,*(test),*(test+1),*(test+2));*/
    
    char* test2 = myallocate(3, "test ", "test" , 2);
    mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    GthreadID = 2;
    *(test2) = 't';
    *(test2+1) = 'u';
    *(test2+2) = 'v';
    //printf("Line b\n");
    printf("test is:%p %c%c%c\n",test2,*(test2),*(test2+1),*(test2+2));
    
    //mprotectFunc(getPhyMem(),8388608,PROT_NONE);
    //GthreadID = 1;
    //printf("test is:%p %c%c%c\n",test2,*(test2),*(test2+1),*(test2+2));
    //char* pm = getPhyMem();
    //int temp = getByteAdditionsForNthPage(0);
    //printf("%c%c%c\n",*(pm+temp),*(pm+temp+1),*(pm+temp+2));
    //printf("test is %p, and pm+temp is %p\n",test,pm+temp);
    
    return 0;
}
