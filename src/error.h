#ifndef QAK_ERROR_H
#define QAK_ERROR_H

#include "source.h"
#include "array.h"

namespace qak {

#define QAK_ERROR(span, ...) { errors.add(span, __VA_ARGS__); return; }

    struct Error {
        Span span;
        const char *message;

        Error(Span span, const char *message) : span(span), message(message) {}

        Line &getLine();

        void print();
    };

    struct Errors {
        HeapAllocator &mem;
        Array<Error> errors;

        Errors(HeapAllocator &mem) : mem(mem), errors(mem) {}

        void add(Error error);

        void add(Span span, const char *msg...);

        Array<Error> &getErrors();

        bool hasErrors();

        void print();
    };
}

#endif //QAK_ERROR_H
