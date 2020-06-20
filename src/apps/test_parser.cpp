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
    Module *module = parser.parse(source, errors);
    QAK_CHECK(module, "Expected module, got nullptr.");
}

void testExpression() {
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_expression.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_expression.qak");
    Source source(file, "data/parser_expression.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");
}

void testModuleVariable() {
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_module_var.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_module_var.qak");
    Source source(file, "data/parser_module_var.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print(mem);
}

void testFunction() {
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_function.qak");
    Source source(file, "data/parser_function.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print(mem);
}

int main() {
    testModule();
    testExpression();
    testModuleVariable();
    testFunction();
    return 0;
}