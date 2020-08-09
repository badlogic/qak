#pragma once

#include "qak.h"

bool qak_span_matches(qak_span *span, const char *needle, size_t size);

void qak_tokenize(qak_source *source, qak_array_token *tokens, qak_array_error *errors);