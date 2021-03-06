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
    int endOffset;
    int allocCounter;
};


const int TotalPages = 2048;
const int BytesPerPage = 4096;
int GthreadID = 0;

int TotalPTRows;
int PTRowsPerPage;
int TotalPagesUsedByPTRows;
int TotalUsablePages;
int remainingFreeFrames;
int AllocationCounter = 0;


void initMemoryStructures(){
    
    TotalPTRows = TotalPages;
    PTRowsPerPage = (BytesPerPage / sizeof(struct PTRow));
    TotalPagesUsedByPTRows = TotalPTRows/PTRowsPerPage;
    TotalUsablePages = TotalPages - TotalPagesUsedByPTRows;
    remainingFreeFrames = TotalPages;
    
    
    //#ifdef __APPLE__
    //physicalMemory = malloc(8388608);
    //#else
    posix_memalign(&physicalMemoryVoidPointer,4096, TotalPages*BytesPerPage);
    physicalMemory = (char*)physicalMemoryVoidPointer;
    //#endif
    
    printf("Physical memory starts from %p\n",physicalMemory);
    printf("page memory starts from %p\n",physicalMemory+TotalPagesUsedByPTRows*BytesPerPage);
    //printf("pagesizee is %d\n",getpagesize());
    //struct PTRow *ptr = physicalMemory;
    int count = 0;
    remainingFreeFrames = TotalPages;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            ptr->isAllocated = 0;
            ptr->PageNum = count++;
            ptr->endOffset = -1;
            ptr->allocCounter = 0;
            //printf("Alloc counter: %d\n\n", ptr->allocCounter);
            //if(ptr->PageNum == 1)
            //    printf("Current Pointer is : %p ::: %p\n",ptr,ptrOut);
        }
        
    }
    
    
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
    {
        printf("Fatal error setting up signal handler\n");
        exit(EXIT_FAILURE);
    }
    
    mprotect( physicalMemory, TotalPages*BytesPerPage, PROT_NONE);
    
    //getMappedPTRow(50);
    
}




void* myallocate(int numberOfBytes,char* fileName,char* lineNumber,int ThreadReq){
    
    mprotect( physicalMemory, 8388608, PROT_READ | PROT_WRITE);
    int numberOfPagesRequested = numberOfBytes/4096 + 1;
    int size =  numberOfBytes;
    
    if(numberOfPagesRequested > remainingFreeFrames){
        return NULL;
    }
    
    int firstFrameFound = -1;
    struct PTRow* ptr = NULL;
    struct PTRow* startPtr = NULL;
    void* pagePointer = NULL;
    
    while (size > 0) {
        int oldOffset = -1;
        ptr = allocateNextFreeFrame(ThreadReq,&size,&oldOffset);
        //ptr->allocCounter++;
        //printf("Allocation count: %d\n", ptr->allocCounter);
        if(firstFrameFound == -1) {
            startPtr = ptr;
            firstFrameFound = 1;
            //pagePointer = getPagePointerFromNumber(startPtr->PageNum);
            pagePointer = physicalMemory + (TotalPagesUsedByPTRows + startPtr->threadBlockNumber)*BytesPerPage;
            pagePointer += oldOffset + 1;
            //printf("Thread block num: %d\n\n", startPtr->threadBlockNumber);
        }
        
    }
    mprotect( physicalMemory, TotalPages*BytesPerPage, PROT_NONE);
    return pagePointer;
}

int mydeallocate(void *freePtr, char *fileName, char *lineNumber, int ThreadID){
    mprotect(physicalMemory, 8388608, PROT_READ | PROT_WRITE);
    int i,j, count=0;
    //printf("Free Pointer: %p\n", freePtr);
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
            char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
            for (j =0; j<PTRowsPerPage; j++) {
                struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            
                if(ptr-> threadID == ThreadID && ptr->isAllocated ==1 && ptr->allocCounter ==1){
                                   
                    remainingFreeFrames++;
                    //printf("nFrames: %d\n", remainingFreeFrames);
                    //printf("Offset: %d\n", ptr->endOffset);
                    
                      //  printf("reached 0\n");
                        ptr -> isAllocated = 0;
                    
                }
                
            }
    }
    mprotect( physicalMemory, TotalPages*BytesPerPage, PROT_NONE);
    return(EXIT_SUCCESS);
}

struct PTRow* allocateNextFreeFrame(int ThreadID,int *numberOfBytes,int *oldOffset){
    
    struct PTRow* lastThreadMemoryPTRow = getLastThreadMemoryPTRow(ThreadID);
    struct PTRow* retVal = NULL;
    int i;
    
    if(lastThreadMemoryPTRow != NULL && lastThreadMemoryPTRow->endOffset != -1){
        int spaceLeftInPage = BytesPerPage - lastThreadMemoryPTRow->endOffset - 1;
        *oldOffset = lastThreadMemoryPTRow->endOffset;
        if(*numberOfBytes >= spaceLeftInPage){
            *numberOfBytes -= spaceLeftInPage;
            lastThreadMemoryPTRow->endOffset = -1;
        }
        else{
            lastThreadMemoryPTRow->endOffset += *numberOfBytes;
            *numberOfBytes = 0;
        }
        retVal = lastThreadMemoryPTRow;
        return retVal;
    }
    else{
        for (i = 0; i<TotalPagesUsedByPTRows; i++) {
            char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
            int j;
            for (j =0; j<PTRowsPerPage; j++) {
                struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            
                if(ptr->isAllocated == 0){
                    ptr->isAllocated = 1;
                    ptr->threadID = ThreadID;
                    ptr->allocCounter++;
                    
                    if(lastThreadMemoryPTRow!=NULL) {
                        ptr->threadBlockNumber = (lastThreadMemoryPTRow->threadBlockNumber)+1;
                    }
                    
                    else{
                        ptr->threadBlockNumber = 0;
                    }
                    *oldOffset = ptr->endOffset;
                    if(*numberOfBytes >= BytesPerPage){
                        ptr->endOffset = -1;
                        *numberOfBytes -= BytesPerPage;
                    }
                    else{
                        ptr->endOffset += *numberOfBytes;
                        *numberOfBytes = 0;
                    }
                    
                    retVal = ptr;
                    remainingFreeFrames--;
                    
                    return retVal;
                }
            }
        }
    }
    
    return retVal;
}

struct PTRow* getLastThreadMemoryPTRow(int ThreadID){
    struct PTRow* retVal = NULL;
    
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            
            if(ptr->isAllocated == 1 && ptr->threadID == ThreadID){
                if(retVal==NULL || (retVal->threadBlockNumber < ptr->threadBlockNumber)){
                    retVal = ptr;
                }
            }
            
        }
        
    }
    
    return retVal;
    
}

struct PTRow* getMappedPTRow(int PageNumber){
    struct PTRow* retVal = (struct PTRow*)(physicalMemory + (PageNumber/PTRowsPerPage)*BytesPerPage + (PageNumber%PTRowsPerPage)*sizeof(struct PTRow));
    //printf("Returned ptrNum: %d\n",retVal->PageNum);
    return retVal;
}

void* getPagePointerFromNumber(int pageNumberOfPTRow){
    
    void* page = physicalMemory+(TotalPagesUsedByPTRows + pageNumberOfPTRow)*BytesPerPage;
    //printf("Page pointer allocated is: %x",page);
    return page;
}


void swapPagesAndPTRows(int pageNum1,int pageNum2){
    
    char tempChar;
    
    char *PG1 = getPagePointerFromNumber(pageNum1);
    char *PG2 = getPagePointerFromNumber(pageNum2);
    
    struct PTRow* PR1 = getMappedPTRow(pageNum1);
    struct PTRow* PR2 = getMappedPTRow(pageNum2);
    
    int i;
    for(i = 0; i < BytesPerPage ; i++){
        tempChar = *(PG1+i);
        *(PG1+i) = *(PG2+i);
        *(PG2+i) = tempChar;
    }
    
    int tempInt;
    
    tempInt = PR1->endOffset;
    PR1->endOffset = PR2->endOffset;
    PR2->endOffset = tempInt;
    
    tempInt = PR1->threadBlockNumber;
    PR1->threadBlockNumber = PR2->threadBlockNumber;
    PR2->threadBlockNumber = tempInt;

    tempInt = PR1->threadID;
    PR1->threadID = PR2->threadID;
    PR2->threadID = tempInt;
    
    tempInt = PR1->isAllocated;
    PR1->isAllocated = PR2->isAllocated;
    PR2->isAllocated = tempInt;
    
}
 

static void handler(int sig, siginfo_t *si, void *unused) {
    //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    mprotect(physicalMemory,BytesPerPage*TotalPages,PROT_READ | PROT_WRITE);
    
    int TID = GthreadID;
    int tBlock = (int)(((char*)si->si_addr - (physicalMemory + TotalPagesUsedByPTRows*BytesPerPage)))/BytesPerPage;
    
    int flag = -1;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            
            if(ptr->isAllocated == 1 && ptr->threadID == TID && ptr->threadBlockNumber == tBlock){
                //mprotect(getPagePointerFromNumber(tBlock),BytesPerPage,PROT_READ | PROT_WRITE);
                //mprotect(getPagePointerFromNumber(ptr->PageNum),BytesPerPage,PROT_READ | PROT_WRITE);
                swapPagesAndPTRows(ptr->PageNum,tBlock);
                mprotect(physicalMemory,BytesPerPage*TotalPages,PROT_NONE);
                mprotect(getPagePointerFromNumber(tBlock),BytesPerPage,PROT_READ | PROT_WRITE);
                flag = 1;
                break;
            }
            
        }
        if(flag == 1) break;
    }
    
    if(flag!=1){
        //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
        exit(0);
    }
    
    
    //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    //exit(0);
}

void mprotectFunc(void *addr, size_t len, int prot){
    mprotect(addr,len,prot);
}


/*int getByteAdditionsForNthPage(int i){
    return (TotalPagesUsedByPTRows + i)*BytesPerPage;
}*/




char* getPhyMem(){
    return physicalMemory;
}











