#ifndef QAK_IO_H
#define QAK_IO_H

#include "source.h"

namespace qak {
    namespace io {
        Source *readFile(const char *fileName, HeapAllocator &mem);

        uint64_t timeMillis();
    };
}

#endif //QAK_IO_H
