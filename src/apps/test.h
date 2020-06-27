#ifndef QAK_TEST_H
#define QAK_TEST_H

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#define QAK_CHECK(expr, ...) { if (!(expr)) { fprintf(stdout, "ERROR: "); fprintf(stdout, __VA_ARGS__); fprintf(stdout, " (%s:%d)\n", __FILE__, __LINE__); fflush(stdout); exit(-1); } }

#ifdef __cplusplus
namespace qak {
    struct Test {
        const char* name;

        Test(const char *name): name(name) {
#ifdef WIN32
            SetConsoleOutputCP(65001);
#endif
            printf("========= Test: %s\n", name);
        }

        ~Test() {
            printf("\n");
        }
    };
}
#endif

#endif
