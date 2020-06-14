#ifndef QAK_TEST_H
#define QAK_TEST_H

#include <cstdio>
#include <cstdlib>
#include "types.h"

#define QAK_CHECK(expr, message) qak::check(expr, message, __FILE__, __LINE__);

namespace qak {
    void check(bool expression, const char* message, const char* file, u4 line) {
        if (expression) return;
        printf("ERROR (%s:%i): %s\n", file, line, message);
        exit(-1);
    }
}

#endif //QAK_TEST_H
