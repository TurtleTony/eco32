/*
 * func.c -- function
 */

#include "common.h"

char *str = "Hello, world! Nr 2\n";

int f(void) {
  char *p;
  int s;

  p = str;
  s = 0;
  while (*p != '\0') {
    s += *p++;
  }
  return s;
}
