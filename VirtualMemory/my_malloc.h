//
//  my_malloc.h
//  VirtualMemory
//
//  Created by Ronil Mehta on 29/03/16.
//  Copyright (c) 2016 Ronil Mehta. All rights reserved.
//

#ifndef __VirtualMemory__my_malloc__
#define __VirtualMemory__my_malloc__

#include <stdio.h>

#endif



//const int TotalBytesUsedByPTRows = 2048 * sizeof(struct PTRow);
//const int OffsetToFirstUsablePage = TotalBytesUsedByPTRows;

void initMemoryStructures();

void* myallocate(int size,char* fileName,char* lineNumber,int ThreadReq);

struct PTRow* allocateNextFreeFrame(int ThreadID);

int getLastThreadBlockNumber(int ThreadID);

struct PTRow* getMappedPTRow(int PageNumber);

void* getPagePointerFromNumber(int pageNumber);

char* getPhyMem();

int getIndexForFirstPage();
//int* mydeallocate(int size,char* fileName,char* lineNumber,int ThreadReq)