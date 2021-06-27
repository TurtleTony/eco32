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

    // Skip duplicate entries
    char **scanner = queue;
    while (scanner < end) {
#ifdef DEBUG
        debugPrintf("      Comparing scanner string '%s' with entry '%s'", *scanner, entry);
#endif
        if (strcmp(*(scanner), entry) == 0) {
#ifdef DEBUG
      debugPrintf("      Skipping duplicate entry '%s'", *scanner);
#endif
            return;
        }
        scanner++;
    }

    *(end++) = entry;
}

char *dequeue(void) {
    return *(start++);
}