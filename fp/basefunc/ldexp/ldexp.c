/*
 * ldexp.c -- implementation and test of ldexp function
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/fp.h"


#define REQ_ULP		0	/* requested accuracy in ulp */


/**************************************************************/


#define EDOM		33
#define ERANGE		34


int errno = 0;


/**************************************************************/


void dump(float x) {
  _FP_Union X;

  X.f = x;
  printf("%e = 0x%08X = [%c%02X.%06X]",
         X.f, X.w, _FP_SGN(X.w) ? '-' : '+',
         _FP_EXP(X.w), _FP_FRC(X.w));
}


/**************************************************************/


int debug = 0;


float fp_ldexp(float x, int n) {
  float y;
  _FP_Union M, N;
  float z;

  y = x;
  if (n > 127) {
    M.w = _FP_FLT(0, 127 + 127, 0);
    y *= M.f;
    n -= 127;
    if (n > 127) {
      y *= M.f;
      n -= 127;
      if (n > 127) {
        n = 127;
      }
    }
  } else
  if (n < -126) {
    M.w = _FP_FLT(0, -126 + 127, 0);
    N.w = _FP_FLT(0, 24 + 127, 0);
    y *= M.f * N.f;
    n += 126 - 24;
    if (n < -126) {
      y *= M.f * N.f;
      n += 126 - 24;
      if (n < -126) {
        n = -126;
      }
    }
  }
  M.w = _FP_FLT(0, n + 127, 0);
  z = y * M.f;
  return z;
}


/**************************************************************/


void test_single(float x, int n) {
  float z, r;

  printf("n = %d\n", n);
  printf("x = ");
  dump(x);
  printf("\n");
  z = fp_ldexp(x, n);
  printf("z = ");
  dump(z);
  printf("\n");
  r = ldexp(x, n);
  printf("r = ");
  dump(r);
  printf("\n");
}


/**************************************************************/


/*
 * This function returns pseudo random numbers uniformly
 * distributed over (0,1).
 */
float ran(void) {
  return (float) rand() / (float) RAND_MAX;
}


/*
 * This function returns pseudo random numbers logarithmically
 * distributed over (1,exp(x)). Thus a*randl(ln(b/a)) is
 * logarithmically distributed in (a,b).
 */
float randl(float x) {
  return exp(x * ran());
}


/**************************************************************/


void test_many(int count, float lbound, float ubound, int maybeNeg) {
  int i;
  int n;
  int below, equal, above, within, ulp;
  float x, z, r;
  _FP_Union Z, R;

  printf("absolute operand interval: (%e,%e)\n", lbound, ubound);
  printf("operands may%sbe negative\n", maybeNeg ? " " : " not ");
  below = 0;
  equal = 0;
  above = 0;
  within = 0;
  ulp = REQ_ULP;
  for (i = 1; i <= count; i++) {
    if (i % 100 == 0) {
      printf("\rnumber of tests: %d", i);
      fflush(stdout);
    }
    x = lbound * randl(log(ubound / lbound));
    if (maybeNeg) {
      if (rand() & 0x400) {
        x = -x;
      }
    }
    n = rand() & 0x0F;
    if (rand() & 0x400) {
      n = -n;
    }
    z = fp_ldexp(x, n);
    r = ldexp(x, n);
    if (z < r) {
      below++;
    } else
    if (z > r) {
      above++;
    } else {
      equal++;
    }
    Z.f = z;
    R.f = r;
    if (_FP_ABS(_FP_DELTA(Z, R)) <= ulp) {
      within++;
    } else {
      printf("++++++++++++++++++++\n");
      printf("n = %d\n", n);
      printf("x = ");
      dump(x);
      printf("\n");
      debug = 1;
      z = fp_ldexp(x, n);
      debug = 0;
      printf("z = ");
      dump(z);
      printf("\n");
      printf("r = ");
      dump(r);
      printf("\n");
      printf("++++++++++++++++++++\n");
    }
  }
  printf("\n");
  printf("below: %d\n", below);
  printf("equal: %d\n", equal);
  printf("above: %d\n", above);
  printf("within error of %d ulp: %d\n", ulp, within);
}


/**************************************************************/


void testFew(void) {
  printf("------------------------------------------------\n");
  test_single(0.0, 0);
  printf("------------------------------------------------\n");
  test_single(0.0, 2);
  printf("------------------------------------------------\n");
  test_single(0.0, -2);
  printf("------------------------------------------------\n");
  test_single(2.3, 0);
  printf("------------------------------------------------\n");
  test_single(2.3, 2);
  printf("------------------------------------------------\n");
  test_single(2.3, -2);
  printf("------------------------------------------------\n");
  test_single(1.2345e2, 0);
  printf("------------------------------------------------\n");
  test_single(1.2345e2, 10);
  printf("------------------------------------------------\n");
  test_single(1.2345e2, -10);
  printf("------------------------------------------------\n");
  test_single(1.2345e-3, 0);
  printf("------------------------------------------------\n");
  test_single(1.2345e-3, 10);
  printf("------------------------------------------------\n");
  test_single(1.2345e-3, -10);
  printf("------------------------------------------------\n");
}


/**************************************************************/


void testMany(void) {
  printf("------------------------------------------------\n");
  test_many(10000000, 0.25, 4.0, 1);
  printf("------------------------------------------------\n");
  test_many(10000000, 1.0e-15, 1.0, 1);
  printf("------------------------------------------------\n");
  test_many(10000000, 1.0, 1.0e15, 1);
  printf("------------------------------------------------\n");
}


/**************************************************************/


void testAll(int skipSome) {
  int ulp;
  unsigned int errors, i;
  int n;
  _FP_Union X, Z, R;

  ulp = REQ_ULP;
  errors = 0;
  i = 0;
  do {
    if ((i & 0x0FFFFFFF) == 0) {
      printf("reached test 0x%08X\n", i);
    }
    if (skipSome) {
      while ((i & 0x0003FFC0) != 0x00000000 &&
             (i & 0x0003FFC0) != 0x0003FFC0) {
        i += 0x00000040;
      }
    }
    X.w = i;
    for (n = -300; n <= 300; n++) {
      Z.f = fp_ldexp(X.f, n);
      R.f = ldexp(X.f, n);
      if (_FP_ABS(_FP_DELTA(Z, R)) > ulp) {
        if (errors == 0) {
          printf("first error at n = %d, X = 0x%08X: Z = 0x%08X, R = 0x%08X\n",
                 n, X.w, Z.w, R.w);
        }
        errors++;
      }
    }
    i++;
  } while (i != 0);
  printf("done, %u errors (not within %d ulp of reference)\n",
         errors, ulp);
}


/**************************************************************/


void usage(char *myself) {
  printf("usage: %s -few|-many|-most|-all\n", myself);
  exit(1);
}


int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage(argv[0]);
  }
  if (strcmp(argv[1], "-few") == 0) {
    testFew();
  } else
  if (strcmp(argv[1], "-many") == 0) {
    testMany();
  } else
  if (strcmp(argv[1], "-most") == 0) {
    testAll(1);
  } else
  if (strcmp(argv[1], "-all") == 0) {
    testAll(0);
  } else {
    usage(argv[0]);
  }
  return 0;
}
