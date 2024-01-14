#define _CRT_SECURE_NO_WARNINGS

#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#include "../Common/Lista.h";

void InitGenericList(NODE** head) {
    *head = NULL;
    (*head)->mutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

}

void GenericListPushAtStart(NODE** head_ref, void* new_data, size_t data_size)
{
    NODE* new_node = (NODE*)malloc(sizeof(NODE));

    new_node->data = malloc(data_size);
    new_node->next = (*head_ref);
    new_node->mutex = CreateMutex(
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

    memcpy(new_node->data, new_data, data_size);

    (*head_ref) = new_node;
}

void PrintGenericList(NODE* node, void (*fptr)(void*))
{
    DWORD dwWaitResult;
    while (node != NULL)
    {
        dwWaitResult = WaitForSingleObject(
            node->mutex,    // handle to mutex
            INFINITE);  // no time-out interval
        if (dwWaitResult == WAIT_OBJECT_0) {  // The thread got ownership of the mutex
            (*fptr)(node->data);
            node = node->next;
            ReleaseMutex(node->mutex);
        }
    }
}

void FreeGenericList(NODE** head) {
    if (*head == NULL) {
        return;
    }

    NODE* next = (*head)->next;
    DWORD dwWaitResult = WaitForSingleObject(
        (*head)->mutex,    // handle to mutex
        INFINITE);  // no time-out interval
    if (dwWaitResult == WAIT_OBJECT_0) {
        CloseHandle((*head)->mutex);
        free((*head)->data);
        (*head)->data = NULL;
        (*head)->next = NULL;
        free(*head);
        *head = NULL;
        //release mutex on null??
    }

    if (next != NULL) {
        FreeGenericList(&next);
    }
}


bool DeleteNode(NODE** head, void* toDelete, size_t size) {
    NODE* current = *head;
    if (current == NULL) {
        return false;
    }

    NODE* prev = NULL;
    char* target = (char*)malloc(size);
    memcpy(target, toDelete, size);
    char* data = (char*)malloc(size);
    while (current != NULL) {
        DWORD dwWaitResult = WaitForSingleObject(
            (current)->mutex,    // handle to mutex
            INFINITE);  // no time-out interval
        if (dwWaitResult == WAIT_OBJECT_0) {
            memcpy(data, current->data, size);
            if (*target == *data) {

                if (prev == NULL) {
                    (*head) = current->next;
                }
                else {
                    prev->next = current->next;
                }
                free(current->data);
                free(current);
                free(data);
                free(target);
                return true;
            }
            ReleaseMutex(current->mutex);
            CloseHandle(current->mutex);
            prev = current;
            current = current->next;
        }

    }
    free(target);
    free(data);
    return false;
}