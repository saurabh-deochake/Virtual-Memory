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

struct PTRow {
    int isAllocated;
    int threadID;
    int threadBlockNumber;
    int PageNum;
    int endOffset;
    int allocationCount;
};

struct allocationData {
    int threadID;
    int firstThreadBlockNumber;
    int length;
    int numberOfPages;
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
    //remainingFreeFrames = TotalPages;
    //SWAP CODE- Made changes here it was = TotalPages
    remainingFreeFrames = TotalUsablePages + TotalSwPages;
    PageNumberOfFirstExtraPTRow = (TotalPhysicalPages - TotalPagesUsedByPTRows - 1);
    
    
    //#ifdef __APPLE__
    //physicalMemory = malloc(8388608);
    //#else
    posix_memalign(&physicalMemoryVoidPointer,4096, TotalPhysicalPages*BytesPerPage);
    physicalMemory = (char*)physicalMemoryVoidPointer;
    //#endif
    
    //SWAP CODE - New Swap file is defined with size 16MB
    swapFile = fopen("swapfile","w+");// Create a file named swapfile with read and write permissions
    ftruncate(fileno(swapFile), TotalSwPages*BytesPerPage); //Size of Swap File 16MB
    fclose(swapFile);
    swapMemoryPointer= swapFile;
    
    printf("Physical memory starts from %p\n",physicalMemory);
    printf("page memory starts from %p\n",physicalMemory+TotalPagesUsedByPTRows*BytesPerPage);

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
            
            //SWAP CODE - Extra Rows of PTRows that are present for the Physical Memory - make them unusable
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
    printf("ThreadVal is %d\n",ThreadReq);
    
    if(hasInitMemoryStructures == -1){
        initMemoryStructures();
        hasInitMemoryStructures = 1;
    }
    
    mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage, PROT_READ | PROT_WRITE | PROT_EXEC);
    int numberOfPagesRequested = numberOfBytes/4096 + 1;
    int size =  numberOfBytes + sizeof(struct allocationData);
    
    if(numberOfPagesRequested > remainingFreeFrames){
        printf("Limit Exceeded");
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
            allocDataPointer = (struct allocationData*)(pagePointer + oldOffset + 1);
            allocDataPointer->length = numberOfBytes;
            allocDataPointer->threadID = ThreadReq;
            allocDataPointer->numberOfPages = 1;
            allocDataPointer->firstThreadBlockNumber = startPtr->threadBlockNumber;
            pagePointer += oldOffset + 1 + sizeof(struct allocationData);
        }
        else{
            allocDataPointer->numberOfPages++;
        }
        
    }
    mprotect( physicalMemory, TotalPhysicalPages*BytesPerPage, PROT_NONE);
    printf("returning pointer: %p\n",pagePointer);
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
                //printf("Reading PageTableRow number: %d, %d\n",ptr->PageNum,ptr->isAllocated);
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
                    //printf("returning: %p\n",ptr);
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
    //printf("Returned ptrNum: %d\n",retVal->PageNum);
    return retVal;
}

void* getPagePointerFromNumber(int pageNumberOfPTRow){
    
    void* page = physicalMemory+(TotalPagesUsedByPTRows + pageNumberOfPTRow)*BytesPerPage;
    //printf("Page pointer allocated is: %x",page);
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
        
        for(i = 0; i < BytesPerPage ; i++){
            tempChar = *(PG1+i);
            *(PG1+i) = *(PG2+i);
            *(PG2+i) = tempChar;
        }
    }
    else {
        printf("Swapping page!\n");
        swapFile = fopen("swapfile","w+");
        int pageNum1New = pageNum1 - TotalPhysicalPages;
        char *PG2 = getPagePointerFromNumber(pageNum2);
        
        for (i = 0; i < BytesPerPage; i++) {
            fseek(swapFile, pageNum1New*BytesPerPage+i, SEEK_SET); //fseek to location of the page
            fread(&tempChar, 1, 1, swapFile);
            fwrite((PG2+i), 1, 1, swapFile);
            *(PG2+i) = tempChar;
        }
        
        fclose(swapFile);
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
    //printf("Entering Handler\n");
    //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_READ | PROT_WRITE | PROT_EXEC);
    //printf("handler1?num: %d\n",(int)(((char*)si->si_addr - (physicalMemory + TotalPagesUsedByPTRows*BytesPerPage))));
    int TID = THREADREQ;
    int tBlock = (int)(((char*)si->si_addr - (physicalMemory + TotalPagesUsedByPTRows*BytesPerPage)))/BytesPerPage;
    //printf("handler2? TID: %d, TBN: %d\n",TID,tBlock);
    int flag = -1;
    int i;
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            //printf("i is %d,j is %d, ptr is %d,%d\n",i,j,ptr->threadID,ptr->PageNum);
            if(ptr->isAllocated == 1 && ptr->threadID == TID && ptr->threadBlockNumber == tBlock){
                //mprotect(getPagePointerFromNumber(tBlock),BytesPerPage,PROT_READ | PROT_WRITE | PROT_EXEC);
                //mprotect(getPagePointerFromNumber(ptr->PageNum),BytesPerPage,PROT_READ | PROT_WRITE | PROT_EXEC);
                //printf("BSwap?\n");
                swapPagesAndPTRows(ptr->PageNum,tBlock);
                //printf("A or B?\n");
                mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_NONE);
                mprotect(getPagePointerFromNumber(tBlock),BytesPerPage,PROT_READ | PROT_WRITE | PROT_EXEC);
                flag = 1;
                break;
            }
            //printf("i is %d,j is %d, ptr is %d,%d\n",i,j,ptr->threadID,ptr->threadBlockNumber);
        }
        if(flag == 1) break;
    }
    
    if(flag!=1){
        printf("Page not found\n");
        exit(0);
    }
    
    //printf("End of Handler\n");
    
    //printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    //exit(0);
}

void mprotectFunc(){    //void *addr, size_t len, int prot){
    //mprotect(addr,len,prot);
    mprotect(physicalMemory,BytesPerPage*TotalPhysicalPages,PROT_NONE);
}


/*int getByteAdditionsForNthPage(int i){
    return (TotalPagesUsedByPTRows + i)*BytesPerPage;
}*/




char* getPhyMem(){
    return physicalMemory;
}


int mydeallocate(void* address,char* fileName,char* lineNumber,int ThreadReq){
    
    struct allocationData* ad = (struct allocationData*)(address - sizeof(struct allocationData));
    int numberOfBytesUsed = ad->length;    // will cause segfault
    int numberOfBytesUsedIncludingAllocData = ad->length + sizeof(struct allocationData);
    int numberOfPagesUsed = ad->numberOfPages;
    int threadIDFromAllocData = ad->threadID;
    int threadBlockNumberFromAllocData = ad->firstThreadBlockNumber;
    
    mprotect( physicalMemory,TotalPhysicalPages*BytesPerPage, PROT_READ | PROT_WRITE | PROT_EXEC);
    
    //int tBlock = (int)(((char*)address - (physicalMemory + TotalPagesUsedByPTRows*BytesPerPage)))/BytesPerPage;
    //int threadID = ThreadReq;
    //int offset = (int)address%BytesPerPage;
    //printf("Offset is: %d",offset);
    struct PTRow* startPtr = NULL;
    
    int i,breakFlag=0;
    
    for (i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        char* ptrOut = (char*)(physicalMemory+i*BytesPerPage);
        int j;
        for (j =0; j<PTRowsPerPage; j++) {
            struct PTRow* ptr = (struct PTRow*)(ptrOut + j*(sizeof(struct PTRow)));
            
            if(ptr->threadID == threadIDFromAllocData && ptr->threadBlockNumber == threadBlockNumberFromAllocData){
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
        struct PTRow* inPtr = startPtr + sizeof(struct PTRow)*i;
        inPtr->allocationCount--;
        if(inPtr->allocationCount == 0){
            inPtr->isAllocated = 0;
            inPtr->endOffset = -1;
            remainingFreeFrames++;
        }
    }
    
    
    mprotect( physicalMemory, TotalPhysicalPages*BytesPerPage, PROT_NONE);
    
    return 0;
}








