#include "skiplist.h"
#include <cstdlib>
#include <cstring>


static int random_level() {
    int level = 1;
    while ((double)rand() / RAND_MAX < SKIPLIST_P &&
           level < SKIPLIST_MAX_LEVEL) {
        level++;
    }
    return level;
}


SkipListNode::SkipListNode(const char* m, size_t mlen, double s, int lvl)
    : score(s), level(lvl)
{
    member  = new char[mlen + 1];
    memcpy(member, m, mlen);
    member[mlen] = '\0';
    forward = new SkipListNode*[lvl]();
}

SkipListNode::~SkipListNode() {
    delete[] member;
    delete[] forward;
}


SkipList::SkipList() : level(1), length(0) {
    header = new SkipListNode("", 0, -1e300, SKIPLIST_MAX_LEVEL);
}

SkipList::~SkipList() {
    SkipListNode* n = header;
    while (n) {
        SkipListNode* next = n->forward[0];
        delete n;
        n = next;
    }
}

bool sl_insert(SkipList* sl, const char* member, size_t mlen, double score) {
    SkipListNode* update[SKIPLIST_MAX_LEVEL];
    SkipListNode* x = sl->header;

    for (int i = sl->level - 1; i >= 0; i--) {
        while (x->forward[i] &&
               (x->forward[i]->score < score ||
               (x->forward[i]->score == score &&
                strcmp(x->forward[i]->member, member) < 0))) {
            x = x->forward[i];
        }
        update[i] = x;
    }

   
    SkipListNode* cur = sl->header->forward[0];
    while (cur) {
        if (strcmp(cur->member, member) == 0) {
         
            sl_delete(sl, member, mlen);
           
            x = sl->header;
            for (int i = sl->level - 1; i >= 0; i--) {
                while (x->forward[i] &&
                       (x->forward[i]->score < score ||
                       (x->forward[i]->score == score &&
                        strcmp(x->forward[i]->member, member) < 0))) {
                    x = x->forward[i];
                }
                update[i] = x;
            }
            break;
        }
        cur = cur->forward[0];
    }

    int lvl = random_level();
    if (lvl > sl->level) {
        for (int i = sl->level; i < lvl; i++)
            update[i] = sl->header;
        sl->level = lvl;
    }

    SkipListNode* n = new SkipListNode(member, mlen, score, lvl);
    for (int i = 0; i < lvl; i++) {
        n->forward[i]         = update[i]->forward[i];
        update[i]->forward[i] = n;
    }

    sl->length++;
    return true;
}

bool sl_delete(SkipList* sl, const char* member, size_t ) {
    SkipListNode* update[SKIPLIST_MAX_LEVEL];
    SkipListNode* x = sl->header;

  
    for (int i = sl->level - 1; i >= 0; i--) {
        while (x->forward[i] &&
               (x->forward[i]->score < 1e300) &&
               strcmp(x->forward[i]->member, member) < 0) {
            x = x->forward[i];
        }
        update[i] = x;
    }

    x = x->forward[0];
    if (!x || strcmp(x->member, member) != 0) return false;

    for (int i = 0; i < sl->level; i++) {
        if (update[i]->forward[i] != x) break;
        update[i]->forward[i] = x->forward[i];
    }

    delete x;
    sl->length--;

    while (sl->level > 1 && !sl->header->forward[sl->level - 1])
        sl->level--;

    return true;
}

bool sl_get_score(SkipList* sl, const char* member, size_t , double* score) {
    SkipListNode* x = sl->header->forward[0];
    while (x) {
        if (strcmp(x->member, member) == 0) {
            *score = x->score;
            return true;
        }
        x = x->forward[0];
    }
    return false;
}


long sl_get_rank(SkipList* sl, const char* member, size_t ) {
    SkipListNode* x    = sl->header->forward[0];
    long          rank = 0;
    while (x) {
        if (strcmp(x->member, member) == 0) return rank;
        rank++;
        x = x->forward[0];
    }
    return -1;
}


int sl_range_by_rank(SkipList* sl, long start, long end,
                     char** members, double* scores, int max) {
  
    if (start < 0) start = (long)sl->length + start;
    if (end   < 0) end   = (long)sl->length + end;
    if (start < 0) start = 0;
    if (end >= (long)sl->length) end = (long)sl->length - 1;
    if (start > end) return 0;

    SkipListNode* x = sl->header->forward[0];
    for (long i = 0; i < start && x; i++) x = x->forward[0];

    int count = 0;
    for (long i = start; i <= end && x && count < max; i++) {
        members[count] = x->member;
        scores[count]  = x->score;
        count++;
        x = x->forward[0];
    }
    return count;
}


size_t sl_length(SkipList* sl) {
    return sl->length;
}
