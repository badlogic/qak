#pragma once

#include "qak.h"

qak_ast_module *qak_parse(qak_source *source, qak_array_token *tokens, qak_errors *errors, qak_allocator *nodeAllocator);
