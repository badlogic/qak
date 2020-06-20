#ifndef QAK_IO_H
#define QAK_IO_H

#include "memory.h"

namespace qak {
    namespace io {
        Buffer readFile(const char *fileName, HeapAllocator &mem);
        u8 timeMillis();
    };
}

#endif //QAK_IO_H
