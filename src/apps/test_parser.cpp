#include <stdio.h>
#include <parser.h>
#include "qak.h"
#include "test.h"

using namespace qak;
using namespace qak::ast;

void testModule() {
    Test test("Parser - simple module");
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
    Test test("Parser - expressions");
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_expression.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_expression.qak");
    Source source(file, "data/parser_expression.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    mem.freeObject(module, __FILE__, __LINE__);
    QAK_CHECK(mem.numAllocations() == 1, "Expected all memory to be deallocated, but %llu allocations remaining.", mem.numAllocations());
}

void testModuleVariable() {
    Test test("Parser - module variable");
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_module_var.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_module_var.qak");
    Source source(file, "data/parser_module_var.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print();
}

void testFunction() {
    Test test("Parser - function");
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_function.qak");
    Source source(file, "data/parser_function.qak");

    Parser parser(mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print();
}

int main() {
    testModule();
    testExpression();
    testModuleVariable();
    testFunction();
    return 0;
}