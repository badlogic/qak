#pragma once

#include "qak.h"

#define QAK_ERROR(errors, span, message) { qak_array_error_add(errors, (qak_error) { { message, sizeof(message) }, span }); return; }
