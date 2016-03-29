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
    int isAllocated;
    int threadID;
    int threadBlockNumber;
    int PageNum;
};

const int TotalPages = 2048;
const int TotalPTRows = TotalPages;
const int BytesPerPage = 4096;
const int PTRowsPerPage = (BytesPerPage / sizeof(struct PTRow));
const int TotalPagesUsedByPTRows = TotalPTRows/PTRowsPerPage;
const int TotalUsablePages = TotalPages - TotalPagesUsedByPTRows;
int remainingFreeFrames = TotalPages;

void initMemoryStructures(){
    printf("Physical memory starts from %x\n",physicalMemory);
    //struct PTRow *ptr = physicalMemory;
    int count = 0;
    remainingFreeFrames = TotalPages;
    for (int i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        struct PTRow *ptr = (struct PTRow*)(physicalMemory+i*BytesPerPage);
        for (int j =0; j<PTRowsPerPage; j++) {
            ptr += j*(sizeof(struct PTRow));
            ptr->isAllocated = 0;
            ptr->PageNum = count++;
        }
        
    }

}


void* myallocate(int numberOfBytes,char* fileName,char* lineNumber,int ThreadReq){
    
    
    int size = numberOfBytes/4096 + 1;
    
    if(size > remainingFreeFrames){
        return NULL;
    }
    
    int firstFrameFound = -1;
    struct PTRow* ptr = NULL;
    void* pagePointer = NULL;
    
    while (size > 0) {
        ptr = allocateNextFreeFrame(ThreadReq);
        
        if(firstFrameFound == -1) {
            pagePointer = getPagePointerFromNumber(ptr->PageNum);
            firstFrameFound = 1;
        }
        
        size--;
    }
    
    return pagePointer;
}


struct PTRow* allocateNextFreeFrame(int ThreadID){
    
    int lastThreadBlockNumber = getLastThreadBlockNumber(ThreadID);
    struct PTRow* retVal = NULL;
    
    for (int i = 0; i<TotalPagesUsedByPTRows; i++) {
        struct PTRow *ptr = (struct PTRow*)(physicalMemory+i*BytesPerPage);
        for (int j =0; j<PTRowsPerPage; j++) {
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
    
    for (int i = 0; i<TotalPagesUsedByPTRows; i++) {
        
        struct PTRow *ptr = (struct PTRow*)(physicalMemory+i*BytesPerPage);
        for (int j =0; j<PTRowsPerPage; j++) {
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












char* getPhyMem(){
    return physicalMemory;
}

int getIndexForFirstPage(){
    return (TotalPagesUsedByPTRows + 0)*BytesPerPage;
}














