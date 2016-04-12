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
#include "my_pthread_t.h"


void* physicalMemoryVoidPointer;
char* physicalMemory;//[8388608]; //memalign( sysconf(_SC_PAGE_SIZE), 8388608);
//[8388608]; //8MB physical Memory

// SWAP FILE CODE

FILE *swapFile, *swapMemoryPointer;
int hasInitMemoryStructures = -1;
int inDealloc = 0;
int inAlloc = 0;

struct PTRow {
    int isAllocated;
    int threadID;
    int threadBlockNumber;
    int PageNum;
    int endOffset;
    int allocationCount;
};




const int TotalPhysicalPages = 2048;
const int BytesPerPage = 4096;
const int TotalSwPages = 4096; //SWAP CODE- No of Pages in Swap file

int GthreadID = 0;

int TotalPages;
int TotalPTRows;
int PTRowsPerPage;
int TotalPagesUsedByPTRows;
int TotalUsablePages;
int remainingFreeFrames;
int PageNumberOfFirstExtraPTRow;



void initMemoryStructures(){
    
    TotalPages = TotalPhysicalPages + TotalSwPages;
    TotalPTRows = TotalPhysicalPages + TotalSwPages;
    PTRowsPerPage = (BytesPerPage / sizeof(struct PTRow));
    TotalPagesUsedByPTRows = TotalPTRows/PTRowsPerPage;
    TotalUsablePages = TotalPhysicalPages - TotalPagesUsedByPTRows;
    remainingFreeFrames = TotalUsablePages + TotalSwPages;
    PageNumberOfFirstExtraPTRow = (TotalPhysicalPages - TotalPagesUsedByPTRows - 1);
    
    posix_memalign(&physicalMemoryVoidPointer,4096, TotalPhysicalPages*BytesPerPage);
    physicalMemory = (char*)physicalMemoryVoidPointer;

    swapFile = fopen("swapfile","rb+");// Create a file named swapfile with read and write permissions
    ftruncate(fileno(swapFile), TotalSwPages*BytesPerPage); //Size of Swap File 16MB
    swapMemoryPointer= swapFile;
    
    int count = 0;
    //remainingFreeFrames = TotalPages;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            ptr->isAllocated = 0;
            ptr->PageNum = count++;
            ptr->endOffset = -1;
            ptr->allocationCount = 0;
            
            if (PageNumberOfFirstExtraPTRow < ptr->PageNum && ptr->PageNum < TotalPhysicalPages) {
                ptr->isAllocated = 1;
                ptr->threadID = -1000;
            }
            
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
    
    mprotect( physicalMemory, TotalPhysicalPages*BytesPerPage, PROT_NONE);
    
    //getMappedPTRow(50);
    
}




void* myallocate(int numberOfBytes,char* fileName,char* lineNumber,int ThreadReq){
    

    setAlarm(0,0);
    inAlloc = 1;
    
    mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage, PROT_READ | PROT_WRITE);
    int numberOfPagesRequested = numberOfBytes/4096 + 1;
    int size =  numberOfBytes + sizeof(struct allocationData);
    
    if(numberOfPagesRequested > remainingFreeFrames){
        printf("Limit Exceeded");
        setAlarm(0,10);
        return NULL;
    }
    
    int firstFrameFound = -1;
    struct PTRow* ptr = NULL;
    struct PTRow* startPtr = NULL;
    struct allocationData* allocDataPointer = NULL;
    void* pagePointer = NULL;
    while (size > 0) {
        int oldOffset = -1;
        ptr = allocateNextFreeFrame(ThreadReq,&size,&oldOffset);
        
        if(firstFrameFound == -1) {
            startPtr = ptr;
            firstFrameFound = 1;
            pagePointer = physicalMemory + (TotalPagesUsedByPTRows + startPtr->threadBlockNumber)*BytesPerPage;
            pagePointer += oldOffset + 1 + sizeof(struct allocationData);
            mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage, PROT_NONE);
            char temp = *((char*)pagePointer);
            mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage, PROT_READ | PROT_WRITE);
            allocDataPointer = (struct allocationData*)((char*)pagePointer - sizeof(struct allocationData));
            allocDataPointer->length = numberOfBytes;
            allocDataPointer->threadID = ThreadReq;
            allocDataPointer->numberOfPages = 1;
            allocDataPointer->firstThreadBlockNumber = startPtr->threadBlockNumber;

        }
        else{
            allocDataPointer->numberOfPages++;
        }
        
    }
    mprotect( physicalMemory, TotalPhysicalPages*BytesPerPage, PROT_NONE);
    
    long currentTimestamp = getCurrentTimestamp();
    currentThread->timeSpent = currentThread->timeSpent + currentTimestamp - currentThread->startTimestamp;
    long quantumAllocation = FIRST_QUEUE_QUANTA - currentThread->timeSpent;
    if (quantumAllocation <= 0) {
        quantumAllocation = 10;
    }
    
    inAlloc = 0;
    setAlarm((int)(quantumAllocation/1000),quantumAllocation%1000);
    
    return pagePointer;
    
}


struct PTRow* allocateNextFreeFrame(int ThreadID,int *numberOfBytes,int *oldOffset){
    
    struct PTRow* lastThreadMemoryPTRow = getLastThreadMemoryPTRow(ThreadID);
    struct PTRow* retVal = NULL;
    int i;
    
    if(lastThreadMemoryPTRow != NULL && lastThreadMemoryPTRow->endOffset != -1 && (BytesPerPage - lastThreadMemoryPTRow->endOffset - 1) > sizeof(struct allocationData)){
        
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
        retVal->allocationCount++;
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
                    retVal->allocationCount++;
                    return retVal;
                }
            }
        }
    }
    retVal->allocationCount++;
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
    return retVal;
}

void* getPagePointerFromNumber(int pageNumberOfPTRow){
    
    void* page = physicalMemory+(TotalPagesUsedByPTRows + pageNumberOfPTRow)*BytesPerPage;
    return page;
}


void swapPagesAndPTRows(int pageNum1,int pageNum2){
    struct PTRow* PR1 = getMappedPTRow(pageNum1);
    struct PTRow* PR2 = getMappedPTRow(pageNum2);
    char tempChar;
    int i;
    if (pageNum1 < TotalPhysicalPages) {
        
        char *PG1 = getPagePointerFromNumber(pageNum1);
        char *PG2 = getPagePointerFromNumber(pageNum2);
        //printf("Swapping: %p and %p\n",PG1,PG2);
        for(i = 0; i < BytesPerPage ; i++){
            tempChar = *(PG1+i);
            *(PG1+i) = *(PG2+i);
            *(PG2+i) = tempChar;
        }
    }
    else {
        
        
        printf("Swapping page from Swapfile!\n");
        //swapFile = fopen("swapfile","w+");
        int pageNum1New = pageNum1 - TotalPhysicalPages;
        char *PG2 = getPagePointerFromNumber(pageNum2);
        for (i = 0; i < BytesPerPage; i++) {
            //rewind(swapFile);
            fseek(swapFile, pageNum1New*BytesPerPage+i, SEEK_SET); //fseek to location of the page
            fread(&tempChar, 1, 1, swapFile);
            fseek(swapFile, pageNum1New*BytesPerPage+i, SEEK_SET); //fseek to location of the page
            fwrite((PG2+i), 1, 1, swapFile);
            *(PG2+i) = tempChar;
        }
        
        //fclose(swapFile);
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
    setAlarm(0,0);
    //printf("Seg Go!!!\n");
    mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_READ | PROT_WRITE);

    int TID = THREADREQ;
    int tBlock = (int)(((char*)si->si_addr - (physicalMemory + sizeof(struct allocationData) + TotalPagesUsedByPTRows*BytesPerPage)))/BytesPerPage;

    int flag = -1;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            //printf("i is %d,j is %d, ptr is %d,%d\n",i,j,ptr->threadID,ptr->PageNum);
            if(ptr->isAllocated == 1 && ptr->threadID == TID && ptr->threadBlockNumber == tBlock){
                swapPagesAndPTRows(ptr->PageNum,tBlock);
                mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_NONE);
                mprotect(getPagePointerFromNumber(tBlock),BytesPerPage,PROT_READ | PROT_WRITE | PROT_EXEC);
                flag = 1;
                break;
            }
        }
        if(flag == 1) break;
    }
    
    if(flag!=1){
        printf("Page not found\n");
        exit(0);
    }
    
    long currentTimestamp = getCurrentTimestamp();
    currentThread->timeSpent = currentThread->timeSpent + currentTimestamp - currentThread->startTimestamp;
    long quantumAllocation = FIRST_QUEUE_QUANTA - currentThread->timeSpent;
    if (quantumAllocation <= 0) {
        quantumAllocation = 10;
    }
    
    if(inDealloc == 0 && inAlloc == 0)  setAlarm((int)(quantumAllocation/1000),quantumAllocation%1000);
    else setAlarm(0,0);
    
}

void mprotectFunc(){
    mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_NONE);
}

void unmprotectFunc(){
    mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_READ | PROT_WRITE);
}




char* getPhyMem(){
    return physicalMemory;
}


int mydeallocate(void* address,char* fileName,char* lineNumber,int ThreadReq){
    inDealloc = 1;
    
    setAlarm(0,0);
    mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage,PROT_NONE);
    //printf("ncoming pointer!! %p and thread %d\n",address,ThreadReq);
    struct allocationData* ad = (struct allocationData*)((char*)address - sizeof(struct allocationData));
    int numberOfBytesUsed = ad->length;    // will cause segfault
    //printf("######numberOfPages is %d, ad address is %p\n",ad->numberOfPages,ad);
    //int numberOfBytesUsedIncludingAllocData = ad->length + sizeof(struct allocationData);
    int numberOfPagesUsed = ad->numberOfPages;
    int threadIDFromAllocData = ThreadReq;
    int threadBlockNumberFromAllocData = ad->firstThreadBlockNumber;
    
    mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage, PROT_READ | PROT_WRITE);
    
    struct PTRow* startPtr = NULL;
    
    int i,breakFlag;
    breakFlag = 0;
    
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            
            if(ptr->threadID == ThreadReq && ptr->threadBlockNumber == threadBlockNumberFromAllocData){
                startPtr = ptr;
                breakFlag = 1;
                break;
            }
        }
        if(breakFlag == 1){
            break;
        }
    }
    
    for(i = 0; i < numberOfPagesUsed ; i++){
        struct PTRow* inPtr = (struct PTRow*)((char*)startPtr + sizeof(struct PTRow)*i);
        inPtr->allocationCount--;
        if(inPtr->allocationCount == 0){
            inPtr->isAllocated = 0;
            inPtr->endOffset = -1;
            remainingFreeFrames++;
        }
    }
    
    
    mprotect( physicalMemory, TotalPhysicalPages*BytesPerPage, PROT_NONE);
    
    long currentTimestamp = getCurrentTimestamp();
    currentThread->timeSpent = currentThread->timeSpent + currentTimestamp - currentThread->startTimestamp;
    long quantumAllocation = FIRST_QUEUE_QUANTA - currentThread->timeSpent;
    if (quantumAllocation <= 0) {
        quantumAllocation = 100;
    }
    
    inDealloc = 0;
    setAlarm((int)(quantumAllocation/1000),quantumAllocation%1000);

    return 0;
}

void shutDown() {
    fclose(swapFile);
}






