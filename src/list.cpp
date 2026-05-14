#include "list.h"
#include <cstdlib>


List::~List() {
    ListNode* n = head;
    while (n) {
        ListNode* next = n->next;
        sds_free(n->value);
        delete n;
        n = next;
    }
}

static long normalize_index(long index, size_t len) {
    if (index < 0) index = (long)len + index;
    return index;
}


void list_lpush(List* l, sds value) {
    ListNode* n = new ListNode(value);
    if (!l->head) {
        l->head = l->tail = n;
    } else {
        n->next       = l->head;
        l->head->prev = n;
        l->head       = n;
    }
    l->len++;
}


void list_rpush(List* l, sds value) {
    ListNode* n = new ListNode(value);
    if (!l->tail) {
        l->head = l->tail = n;
    } else {
        n->prev       = l->tail;
        l->tail->next = n;
        l->tail       = n;
    }
    l->len++;
}


sds list_lpop(List* l) {
    if (!l->head) return nullptr;
    ListNode* n   = l->head;
    sds       val = n->value;
    l->head       = n->next;
    if (l->head) l->head->prev = nullptr;
    else         l->tail = nullptr;
    delete n;
    l->len--;
    return val;
}


sds list_rpop(List* l) {
    if (!l->tail) return nullptr;
    ListNode* n   = l->tail;
    sds       val = n->value;
    l->tail       = n->prev;
    if (l->tail) l->tail->next = nullptr;
    else         l->head = nullptr;
    delete n;
    l->len--;
    return val;
}


size_t list_len(List* l) {
    return l->len;
}


sds list_index(List* l, long index) {
    index = normalize_index(index, l->len);
    if (index < 0 || index >= (long)l->len) return nullptr;

    ListNode* n;
  
    if (index < (long)l->len / 2) {
        n = l->head;
        for (long i = 0; i < index; i++) n = n->next;
    } else {
        n = l->tail;
        for (long i = (long)l->len - 1; i > index; i--) n = n->prev;
    }
    return n->value;
}


int list_range(List* l, long start, long end, sds* out, int max_out) {
    start = normalize_index(start, l->len);
    end   = normalize_index(end,   l->len);

    if (start < 0) start = 0;
    if (end   >= (long)l->len) end = (long)l->len - 1;
    if (start > end) return 0;

    int       count = 0;
    ListNode* n     = l->head;

    for (long i = 0; i < start; i++) n = n->next;
    for (long i = start; i <= end && count < max_out; i++) {
        out[count++] = n->value;
        n = n->next;
    }
    return count;
}