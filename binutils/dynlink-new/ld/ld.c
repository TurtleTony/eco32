/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

#include "ld.h"

int main(int argc, char *argv[]) {

    char *infile = NULL;
    char *outfile = DEFAULT_OUT_FILE_NAME;

    Module *mod;

    int c;
    // Get-opt keeps this easily expandable for the future
    while ((c = getopt(argc, argv, "o:?")) != -1) {
        switch (c) {
            case 'o':
                outfile = optarg;
                break;
            case '?':
                printUsage(argv[0]);
                return 1;
            default:
                return 1;
        }
    }

    // TODO: turn into loop, parsing multiple input files
    if (optind >= argc) {
        // Require an additional option for the infile
        printUsage(argv[0]);
        return 1;
    }

    infile = argv[optind];

    mod = readModule(infile);
    writeModule(mod, outfile);
    return 0;
}


/**************************************************************/
/** HELPER */
/**************************************************************/


void printUsage(char *arg0) {
    printf("usage: %s [-o <outfile>] <infile>\n", arg0);
}


void error(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "!\n");
    va_end(ap);
    exit(1);
}


void *safeAlloc(unsigned int size) {
    void *p;

    p = malloc(size);
    if (p == NULL) {
        error("unable to allocate %d bytes", size);
    }
    return p;
}


void safeFree(void *p) {
    if (p == NULL) {
        error("tried to free NULL pointer");
    }
    free(p);
}
