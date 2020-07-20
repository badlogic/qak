#include <stdio.h>
#include "qak.h"
#include "test.h"

int main() {
    qak_compiler compiler = qak_compiler_new();
    QAK_CHECK(compiler, "Couldn't create compiler");

    qak_module module = qak_compiler_compile_file(compiler, "data/parser_function.qak");
    QAK_CHECK(module, "Couldn't parse module");
    qak_module_delete(module);

    qak_compiler_delete(compiler);
}
