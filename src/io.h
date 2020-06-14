#ifndef QAK_IO_H
#define QAK_IO_H

#include "memory.h"

namespace qak {
    Buffer readFile(const char *fileName, MemoryArea &mem);
}

#endif //QAK_IO_H
