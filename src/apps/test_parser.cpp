#include <stdio.h>
#include <parser.h>
#include "qak.h"
#include "test.h"

using namespace qak;
using namespace qak::ast;

void testBench() {
    Test test("Parser - Benchmark");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_benchmark.qak");
    Source source(file, "data/parser_benchmark.qak");

    u8 start = io::timeMillis();
    Parser parser(bumpMem, mem);
    Errors errors(mem);

    u4 iterations = 1000000;
    for (u4 i = 0; i < iterations; i++) {
        Module *module = parser.parse(source, errors);
        if (errors.hasErrors()) errors.print();
        QAK_CHECK(module, "Expected module, got nullptr.");
    }

    f8 time = (io::timeMillis() - start) / 1000.0;
    f8 throughput = (f8) file.size * iterations / time / 1024 / 1024;
    printf("File size: %llu bytes\n", file.size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);
}

void testModule() {
    Test test("Parser - simple module");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_module.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_module.qak");
    Source source(file, "data/parser_module.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    QAK_CHECK(module, "Expected module, got nullptr.");
}

void testExpression() {
    Test test("Parser - expressions");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_expression.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_expression.qak");
    Source source(file, "data/parser_expression.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    mem.freeObject(module, __FILE__, __LINE__);
    QAK_CHECK(mem.numAllocations() == 1, "Expected all memory to be deallocated, but %llu allocations remaining.", mem.numAllocations());
}

void testModuleVariable() {
    Test test("Parser - module variable");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_module_var.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_module_var.qak");
    Source source(file, "data/parser_module_var.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print();
}

void testFunction() {
    Test test("Parser - function");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Buffer file = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(file.data != nullptr, "Couldn't read test file data/parser_function.qak");
    Source source(file, "data/parser_function.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print();
}

int main() {
    testBench();
    /*testModule();
    testExpression();
    testModuleVariable();
    testFunction();*/
    return 0;
}