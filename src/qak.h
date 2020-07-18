#ifndef QAK_C_API_H
#define QAK_C_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define QAK_TRUE 1
#define QAK_FALSE 0

typedef void *qak_compiler;
typedef void *qak_module;

qak_compiler qak_compiler_new();

void qak_compiler_delete(qak_compiler compiler);

qak_module qak_compile_file(qak_compiler compiler, const char *fileName);

qak_module qak_compile_source(qak_compiler compiler, const char *fileName, const char *source);

void qak_module_delete(qak_module module);

int qak_module_has_errors(qak_module module);

void qak_module_print_errors(qak_module module);

void qak_module_print_tokens(qak_module module);

void qak_module_print_ast(qak_module module);

int qak_version();

#ifdef __cplusplus
}
#endif

#endif
