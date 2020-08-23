#pragma once

#include "qak.h"

qak_source *qak_io_read_source_from_file(qak_allocator *allocator, const char *fileName);

qak_source *qak_io_read_source_from_memory(qak_allocator *allocator, const char *fileName, const char *sourceCode);

void qak_source_delete(qak_source *source);

void qak_source_get_lines(qak_source *source);

double qak_io_time_millis();
