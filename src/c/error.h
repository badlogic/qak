#pragma once

#include "qak.h"

qak_errors qak_errors_init(qak_allocator *allocator);

void qak_errors_shutdown(qak_errors *errors);

void qak_errors_add(qak_errors *errors, qak_source *source, qak_span span, const char *msg, ...);

void qak_errors_print(qak_errors *errors);

void qak_error_print(qak_error *error);
