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
        int newSize = size * 2;
#ifdef DEBUG
        debugPrintf("      Queue overflow for size '%d', allocating more memory for new size '%d'", size, newSize);
#endif
        char ** newQueue = safeAlloc(newSize);
        memcpy(newQueue, queue, size);

        size = newSize;
        queue = newQueue;
    }



    *(end++) = entry;
}

char *dequeue(void) {
    return *(start++);
}