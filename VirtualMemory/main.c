//
//  main.c
//  VirtualMemory
//
//  Created by Ronil Mehta on 29/03/16.
//  Copyright (c) 2016 Ronil Mehta. All rights reserved.
//

#include <stdio.h>
#include "my_malloc.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    printf("Hello, World!\n");
    
    initMemoryStructures();
    
    char* test = myallocate(3, "test ", "test" , 1);
    *(test) = 'a';
    *(test+1) = 'b';
    *(test+2) = 'c';
    
    printf("%c%c%c\n",*(test),*(test+1),*(test+2));
    
    char* pm = getPhyMem();
    int temp = getIndexForFirstPage();
    printf("%c%c%c\n",*(pm+temp),*(pm+temp+1),*(pm+temp+2));
    char* tt = pm+temp;
    printf("test is %x, and pm+temp is %x\n",test,tt);
    
    return 0;
}
