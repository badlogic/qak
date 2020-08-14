#include "error.h"
#include "io.h"
#include <stdio.h>
#include <stdarg.h>

void qak_errors_print(qak_errors *errors) {
    for (size_t i = 0; i < errors->errors->size; i++)
        qak_error_print(&errors->errors->items[i]);
}

void qak_error_print(qak_error *error) {
    qak_source *source = error->source;
    qak_source_get_lines(source);
    qak_line *line = &source->lines->items[error->span.startLine];

    printf("Error (%.*s:%i): %.*s\n", (int) source->fileName.length, source->fileName.data, line->lineNumber, (int) error->errorMessage.length,
           error->errorMessage.data);

    if (line->data.length > 0) {
        printf("%.*s\n", (int) line->data.length, line->data.data);
        int32_t errorStart = (int32_t)(error->span.data.data - line->data.data);
        int32_t errorEnd = errorStart + (int32_t)error->span.data.length - 1;
        for (int32_t i = 0, n = (int32_t)line->data.length; i < n; i++) {
            bool useTab = line->data.data[i] == '\t';
            printf("%s", i >= errorStart && i <= errorEnd ? "^" : (useTab ? "\t" : " "));
        }
        printf("\n");
    }
}

qak_errors qak_errors_init(qak_allocator *allocator) {
    return (qak_errors) {allocator, qak_array_error_new(allocator, 16)};
}

void qak_errors_shutdown(qak_errors *errors) {
    for (size_t i = 0; i < errors->errors->size; i++) {
        QAK_FREE(errors->allocator, errors->errors->items[i].errorMessage.data);
    }
    qak_array_error_delete(errors->errors);
}

void qak_errors_add(qak_errors *errors, qak_source *source, qak_span span, const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    char scratch[1];
    int len = vsnprintf(scratch, 1, msg, args);
    va_end(args);

    char *buffer = QAK_ALLOCATE(errors->allocator, char, len + 1);
    va_start(args, msg);
    vsnprintf(buffer, len + 1, msg, args);
    va_end(args);

    qak_array_error_add(errors->errors, (qak_error) {source, {buffer, len}, span});
}
