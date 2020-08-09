#pragma once

#include "qak.h"

qak_source *qak_io_read_source_from_file(qak_allocator *allocator, const char *fileName);

void qak_source_delete(qak_source *source);

double qak_io_time_millis();
