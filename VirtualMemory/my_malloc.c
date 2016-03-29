//
//  my_malloc.c
//  VirtualMemory
//
//  Created by Ronil Mehta on 29/03/16.
//  Copyright (c) 2016 Ronil Mehta. All rights reserved.
//

#include "my_malloc.h"


char physicalMemory[8388608]; //8MB physical Memory

struct PTRow {
    int threadID;
    int threadBlockNumber;
};

const int TotalPages = 2048;
const int TotalPTRows = TotalPages;
const int PTRowsPerPage = (4096 / sizeof(struct PTRow));
const int TotalPagesUsedByPTRows = TotalPTRows/PTRowsPerPage;
const int TotalUsablePages = TotalPages - TotalPagesUsedByPTRows;


int* myallocate(int size,char* fileName,char* lineNumber,int ThreadReq){

    


    return 0;
}