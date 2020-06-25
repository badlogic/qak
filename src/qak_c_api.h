#ifndef QAK_C_API_H
#define QAK_C_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *qak_compiler;
typedef void *qak_module;

qak_compiler qak_compiler_new();

void qak_compiler_delete(qak_compiler compiler);

qak_module qak_compile_file(qak_compiler compiler, const char *fileName);

void qak_module_delete(qak_module moduleHandle);

#ifdef __cplusplus
}
#endif

#endif
