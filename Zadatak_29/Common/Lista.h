#pragma once

#include<stdio.h> 
#include<stdlib.h> 
#include<windows.h>

#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

typedef struct Node
{
    // Any data type can be stored in this node 
    void* data;
    HANDLE mutex;
    struct Node* next;
}NODE;
