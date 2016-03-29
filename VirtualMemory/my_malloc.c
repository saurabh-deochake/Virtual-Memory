//
//  my_malloc.c
//  VirtualMemory
//
//  Created by Ronil Mehta on 29/03/16.
//  Copyright (c) 2016 Ronil Mehta. All rights reserved.
//

#include "my_malloc.h"
#include <stdlib.h>
#include <sys/mman.h>

void* physicalMemoryVoidPointer;
char* physicalMemory;//[8388608]; //memalign( sysconf(_SC_PAGE_SIZE), 8388608);
//[8388608]; //8MB physical Memory

struct PTRow {
    int isAllocated;
    int threadID;
    int threadBlockNumber;
    int PageNum;
};

const int TotalPages = 2048;
const int BytesPerPage = 4096;

int TotalPTRows;
int PTRowsPerPage;
int TotalPagesUsedByPTRows;
int TotalUsablePages;
int remainingFreeFrames;



void initMemoryStructures(){
    
    TotalPTRows = TotalPages;
    PTRowsPerPage = (BytesPerPage / sizeof(struct PTRow));
    TotalPagesUsedByPTRows = TotalPTRows/PTRowsPerPage;
    TotalUsablePages = TotalPages - TotalPagesUsedByPTRows;
    remainingFreeFrames = TotalPages;
    
    
    //#ifdef __APPLE__
    //physicalMemory = malloc(8388608);
    //#else
    posix_memalign(&physicalMemoryVoidPointer,4096, 8388608);
    physicalMemory = (char*)physicalMemoryVoidPointer;
    //#endif
    
    printf("Physical memory starts from %p\n",physicalMemory);
    //printf("pagesizee is %d\n",getpagesize());
    //struct PTRow *ptr = physicalMemory;
    int count = 0;
    remainingFreeFrames = TotalPages;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        struct PTRow *ptr = (struct PTRow*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            ptr += j*(sizeof(struct PTRow));
            ptr->isAllocated = 0;
            ptr->PageNum = count++;
        }
        
    }
    
    mprotect( physicalMemory, 4096, PROT_NONE);
    
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
    {
        printf("Fatal error setting up signal handler\n");
        exit(EXIT_FAILURE);
    }
    
}

static void handler(int sig, siginfo_t *si, void *unused)
{
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    exit(0);
}


void* myallocate(int numberOfBytes,char* fileName,char* lineNumber,int ThreadReq){
    
    
    
    int size = numberOfBytes/4096 + 1;
    
    if(size > remainingFreeFrames){
        return NULL;
    }
    
    int firstFrameFound = -1;
    struct PTRow* ptr = NULL;
    struct PTRow* startPtr = NULL;
    void* pagePointer = NULL;
    
    while (size > 0) {
        ptr = allocateNextFreeFrame(ThreadReq);
        
        if(firstFrameFound == -1) {
            //pagePointer = getPagePointerFromNumber(ptr->PageNum);
            startPtr = ptr;
            firstFrameFound = 1;
        }
        
        size--;
    }
    pagePointer = getPagePointerFromThreadBlockNumber(startPtr->threadBlockNumber);
    return pagePointer;
}


struct PTRow* allocateNextFreeFrame(int ThreadID){
    
    int lastThreadBlockNumber = getLastThreadBlockNumber(ThreadID);
    struct PTRow* retVal = NULL;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        struct PTRow *ptr = (struct PTRow*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            ptr += j*(sizeof(struct PTRow));
            
            if(ptr->isAllocated == 0){
                ptr->isAllocated = 1;
                ptr->threadID = ThreadID;
                ptr->threadBlockNumber = ++lastThreadBlockNumber;
                retVal = ptr;
                remainingFreeFrames--;
                return retVal;
            }
        }
    }
    
    return retVal;
}

int getLastThreadBlockNumber(int ThreadID){
    int retVal = -1;
    
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        struct PTRow *ptr = (struct PTRow*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            ptr += j*(sizeof(struct PTRow));
            
            if(ptr->isAllocated == 1 && ptr->threadID == ThreadID){
                if(retVal < ptr->threadBlockNumber){
                    retVal = ptr->threadBlockNumber;
                }
            }
            
        }
        
    }
    
    return retVal;
    
}

struct PTRow* getMappedPTRow(int PageNumber){
    
    struct PTRow* retVal;

    return retVal;
}

void* getPagePointerFromNumber(int pageNumber){
    
    void* page = physicalMemory+(TotalPagesUsedByPTRows + pageNumber)*BytesPerPage;
    //printf("Page pointer allocated is: %x",page);
    return page;
}

int getByteAdditionsForNthPage(int i){
    return (TotalPagesUsedByPTRows + i)*BytesPerPage;
}

void* getPagePointerFromThreadBlockNumber(int threadBlockNumber){
    void* pageP = physicalMemory + (TotalPagesUsedByPTRows + threadBlockNumber)*BytesPerPage;
    return pageP;
}




char* getPhyMem(){
    return physicalMemory;
}
















