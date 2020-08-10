#pragma once

#include "qak.h"

#define QAK_ERROR(errors, span, message) { qak_array_error_add(errors, (qak_error) { { message, sizeof(message) }, span }); return; }

qak_error qak_error_from_string(qak_allocator *allocator, qak_span *span, const char *msg, ...);
