/*
 * ld.c -- ECO32 link editor
 * Created by tony on 20.04.21.
 */

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

#include "eof.h"
#include "ld.h"

int main(int argc, char *argv[]) {

    char *infile = NULL;
    char *outfile = DEFAULT_OUT_FILE_NAME;

    int c;
    // Getopt keeps this easily expandable for the future
    while ((c = getopt(argc, argv, "o:?")) != -1) {
        switch (c) {
            case 'o':
                outfile = optarg;
                break;
            case '?':
                print_usage(argv[0]);
                return 1;
            default:
                return 1;
        }
    }

    if (optind >= argc) {
        // Require an additional option for the infile
        print_usage(argv[0]);
        return 1;
    }

    infile = argv[optind];

    printf("Hello World!\n");
    return 0;
}

void print_usage(char *arg0) {
    printf("usage: %s [-o <outfile>] <infile>\n", arg0);
}