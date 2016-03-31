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
#include <signal.h>

#endif



//const int TotalBytesUsedByPTRows = 2048 * sizeof(struct PTRow);
//const int OffsetToFirstUsablePage = TotalBytesUsedByPTRows;

void initMemoryStructures();

static void handler(int sig, siginfo_t *si, void *unused);

void* myallocate(int size,char* fileName,char* lineNumber,int ThreadReq);

struct PTRow* allocateNextFreeFrame(int ThreadID,int *numberOfBytes,int *oldOffset);

struct PTRow* getLastThreadMemoryPTRow(int ThreadID);

struct PTRow* getMappedPTRow(int PageNumber);

void* getPagePointerFromNumber(int pageNumber);

void swapPagesAndPTRows(int pageNum1,int pageNum2);

static void handler(int sig, siginfo_t *si, void *unused);

//int getByteAdditionsForNthPage();

//int* mydeallocate(int size,char* fileName,char* lineNumber,int ThreadReq)


//char* getPhyMem();



