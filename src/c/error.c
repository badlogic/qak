#include "error.h"
#include "io.h"
#include <stdio.h>
#include <stdarg.h>

void qak_error_print(qak_source *source, qak_error *error) {
    qak_allocator mem;
    qak_allocator_init(&mem);

    qak_source_get_lines(source);
    qak_line *line = &source->lines->items[error->span.startLine];

    printf("Error (%.*s:%i): %.*s\n", (int)source->fileName.length, source->fileName.data, line->lineNumber, (int)error->errorMessage.length, error->errorMessage.data);

    if (line->data.length > 0) {
        printf("%.*s\n", (int)line->data.length, line->data.data);
        int32_t errorStart = error->span.data.data - line->data.data;
        int32_t errorEnd = errorStart + error->span.data.length - 1;
        for (int32_t i = 0, n = line->data.length; i < n; i++) {
            bool useTab = line->data.data[i] == '\t';
            printf("%s", i >= errorStart && i <= errorEnd ? "^" : (useTab ? "\t" : " "));
        }
        printf("\n");
    }
}

qak_error qak_error_from_string(qak_allocator *allocator, qak_span *span, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    char scratch[1];
    int len = vsnprintf(scratch, 1, msg, args);
    char *buffer = QAK_ALLOCATE(allocator, char, len + 1);
    va_end(args);

    va_start(args, msg);
    vsnprintf(buffer, len + 1, msg, args);
    va_end(args);
    return (qak_error){{buffer, len}, *span};
}
