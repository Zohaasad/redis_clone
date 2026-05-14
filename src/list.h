#pragma once

#include "sds.h"
#include <cstddef>


struct ListNode {
    sds       value;
    ListNode* prev;
    ListNode* next;

    explicit ListNode(sds v) : value(v), prev(nullptr), next(nullptr) {}
};


struct List {
    ListNode* head;   
    ListNode* tail;   
    size_t    len;

    List() : head(nullptr), tail(nullptr), len(0) {}
    ~List();
};


void  list_lpush(List* l, sds value);       
void  list_rpush(List* l, sds value);      
sds   list_lpop(List* l);                  
sds   list_rpop(List* l);                 
size_t list_len(List* l);                   
sds   list_index(List* l, long index);       
int   list_range(List* l, long start, long end, sds* out, int max_out);