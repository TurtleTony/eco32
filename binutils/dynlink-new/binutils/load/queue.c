//
// Created by tony on 27.06.21.
//

#include "queue.h"

char **queue = NULL;
char **start = NULL, **end = NULL;
int size = DEFAULT_SIZE;

void enqueue(char *entry) {
    if (queue == NULL) {
        queue = safeAlloc(size);
        start = queue;
        end = queue;
    }

    if (end >= queue + size) {
        error("queue overflow, implement dynamic size?");
    }



    *(end++) = entry;
}

char *dequeue(void) {
    return *(start++);
}