#include "error.h"
#include "io.h"
#include <stdio.h>

void qak_error_print(qak_source *source, qak_error *error) {
    qak_allocator mem;
    qak_allocator_init(&mem);

    qak_source_get_lines(source);
    qak_line *line = &source->lines->items[error->span.startLine];

    printf("Error (%.*s:%i): %.*s\n", source->fileName.length, source->fileName.data, line->lineNumber, error->errorMessage.length, error->errorMessage.data);

    if (line->data.length > 0) {
        printf("%.*s\n", line->data.length, line->data.data);
        int32_t errorStart = error->span.data.data - line->data.data;
        int32_t errorEnd = errorStart + error->span.data.length - 1;
        for (int32_t i = 0, n = line->data.length; i < n; i++) {
            bool useTab = line->data.data[i] == '\t';
            printf("%s", i >= errorStart && i <= errorEnd ? "^" : (useTab ? "\t" : " "));
        }
    }
}