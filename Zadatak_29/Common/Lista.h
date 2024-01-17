#pragma once

#include<stdio.h> 
#include<stdlib.h> 
#include<windows.h>

#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

typedef struct Node
{
    void* data;
    struct Node* next;
}NODE;
