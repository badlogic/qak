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

    Source *source = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_benchmark.qak");

    uint64_t start = io::timeMillis();
    Parser parser(bumpMem, mem);
    Errors errors(mem);

    uint32_t iterations = 1000000;
    for (uint32_t i = 0; i < iterations; i++) {
        Module *module = parser.parse(*source, errors);
        if (errors.hasErrors()) errors.print();
        QAK_CHECK(module, "Expected module, got nullptr.");
    }

    double time = (io::timeMillis() - start) / 1000.0;
    double throughput = (double) source->size * iterations / time / 1024 / 1024;
    printf("File size: %zu bytes\n", source->size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);
}

void testModule() {
    Test test("Parser - simple module");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_module.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_module.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(*source, errors);
    QAK_CHECK(module, "Expected module, got nullptr.");
}

void testExpression() {
    Test test("Parser - expressions");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_expression.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_expression.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(*source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    mem.freeObject(module, __FILE__, __LINE__);
    if (mem.numAllocations() != 3) mem.printAllocations();
    QAK_CHECK(mem.numAllocations() == 3, "Expected all memory to be deallocated, but %zu allocations remaining.", mem.numAllocations());
}

void testModuleVariable() {
    Test test("Parser - module variable");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_module_var.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_module_var.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(*source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print(mem);
}

void testFunction() {
    Test test("Parser - function");
    BumpAllocator bumpMem;
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_function.qak");

    Parser parser(bumpMem, mem);
    Errors errors(mem);
    Module *module = parser.parse(*source, errors);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    module->print(mem);
}

int main() {
    testBench();
    testModule();
    testExpression();
    testModuleVariable();
    testFunction();
    return 0;
}