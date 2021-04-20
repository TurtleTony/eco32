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

    Module *mod;

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

    mod = readModule(infile);
    writeModule(mod, outfile);
    return 0;
}

void print_usage(char *arg0) {
    printf("usage: %s [-o <outfile>] <infile>\n", arg0);
}


/**************************************************************/


Module *readModule(char *infile) {
    return NULL;
}

void writeModule(Module *module, char *outfile) {

}

void print_usage(char *arg0) {
    printf("usage: %s [-o <outfile>] <infile>\n", arg0);
}


void conv4FromEcoToNative(unsigned char *p) {
    unsigned int data;

    data = read4FromEco(p);
    * (unsigned int *) p = data;
}


void conv4FromNativeToEco(unsigned char *p) {
    unsigned int data;

    data = * (unsigned int *) p;
    write4ToEco(p, data);
}


/**************************************************************/

