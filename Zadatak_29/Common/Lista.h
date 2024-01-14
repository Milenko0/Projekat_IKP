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

/*
void InitGenericList(NODE** head);


void GenericListPushAtStart(NODE** head_ref, void* new_data, size_t data_size);

void PrintGenericList(NODE* node, void (*fptr)(void*));



void FreeGenericList(NODE** head);


bool DeleteNode(NODE** head, void* toDelete, size_t size);*/