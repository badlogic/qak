#include <stdio.h>
#include <parser.h>
#include "qak.h"
#include "test.h"

using namespace qak;
using namespace qak::ast;

void testModule() {
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_module.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_module.qak");
    Source source(file, "data/parser_module.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module* module = parser.parse(source, errors);
    QAK_CHECK(module, "Expected module, got nullptr.");
}

void testModuleVariable() {
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_module_var.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_module_var.qak");
    Source source(file, "data/parser_module_var.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module* module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    for (u8 i = 0; i < module->variables.getSize(); i++) {
        Variable* var = module->variables[i];
        printf("variable %s:\n", var->name.toCString(mem));
        printf("  type: %s\n", var->type ? var->type->name.toCString(mem) : "<unknown>");
        printf("  value: %s\n", var->expression ? var->expression->span.toCString(mem) : "<unknown>");
    }
}

int main() {
    testModule();
    testModuleVariable();
    return 0;
}