//
// Created by tony on 20.04.21.
//

#ifndef ECO32_LD_H
#define ECO32_LD_H



#define DEFAULT_OUT_FILE_NAME "a.out"

typedef struct module {
} Module;

Module *readModule(char *infile);
void writeModule(Module *module, char *outfile);
void print_usage(char *arg0);

#endif //ECO32_LD_H
