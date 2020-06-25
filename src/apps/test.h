#ifndef QAK_TEST_H
#define QAK_TEST_H

#include <stdio.h>
#include <stdlib.h>

#define QAK_CHECK(expr, ...) { if (!(expr)) { fprintf(stdout, "ERROR: "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, " (%s:%d)\n", __FILE__, __LINE__); fflush(stdout); exit(-1); } }

#endif
