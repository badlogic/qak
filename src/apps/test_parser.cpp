#include <stdio.h>
#include "io.h"
#include "parser.h"
#include "test.h"

using namespace qak;
using namespace qak::ast;

void testBench() {
    Test test("Parser - Benchmark");
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_benchmark.qak");

    double start = io::timeMillis();
    Parser parser(mem);
    Errors errors(mem);

    printf("Total allocations before benchmark: %zu\n", mem.totalAllocations());

    uint32_t iterations = 1000000;
    for (uint32_t i = 0; i < iterations; i++) {
        BumpAllocator moduleMem;
        Module *module = parser.parse(*source, errors, &moduleMem);

        if (errors.hasErrors()) errors.print();
        QAK_CHECK(module, "Expected module, got nullptr.");
    }

    double time = (io::timeMillis() - start) / 1000.0;
    double throughput = (double) source->size * iterations / time / 1024 / 1024;
    printf("File size: %zu bytes\n", source->size);
    printf("Took %f\n", time);
    printf("Throughput %f MB/s\n", throughput);

    printf("Total allocations after benchmark: %zu\n", mem.totalAllocations());
    printf("Total frees: %zu\n", mem.totalFrees());
    printf("Allocations after benchmark: %zu\n", mem.numAllocations());
}

void testModule() {
    Test test("Parser - simple module");
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_module.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_module.qak");

    Parser parser(mem);
    Errors errors(mem);
    BumpAllocator moduleMem;
    Module *module = parser.parse(*source, errors, &moduleMem);
    QAK_CHECK(module, "Expected module, got nullptr.");
}

void testExpression() {
    Test test("Parser - expressions");
    HeapAllocator mem;

    {
        Source *source = io::readFile("data/parser_expression.qak", mem);
        QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_expression.qak");
        Parser parser(mem);
        Errors errors(mem);
        BumpAllocator moduleMem;
        Module *module = parser.parse(*source, errors, &moduleMem);
        if (errors.hasErrors()) errors.print();
        QAK_CHECK(module, "Expected module, got nullptr.");
        module->~Module();
        mem.freeObject(source, QAK_SRC_LOC);
    }

    if (mem.numAllocations() != 0) mem.printAllocations();
    QAK_CHECK(mem.numAllocations() == 0, "Expected all memory to be deallocated, but %zu allocations remaining.", mem.numAllocations());
}

void testModuleVariable() {
    Test test("Parser - module variable");
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_module_var.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_module_var.qak");

    Parser parser(mem);
    Errors errors(mem);
    BumpAllocator moduleMem;
    Module *module = parser.parse(*source, errors, &moduleMem);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    parser::printAstNode(module, mem);
}

void testFunction() {
    Test test("Parser - function");
    HeapAllocator mem;

    Source *source = io::readFile("data/parser_function.qak", mem);
    QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_function.qak");

    Parser parser(mem);
    Errors errors(mem);
    BumpAllocator moduleMem;
    Module *module = parser.parse(*source, errors, &moduleMem);
    if (errors.hasErrors()) errors.print();
    QAK_CHECK(module, "Expected module, got nullptr.");

    parser::printAstNode(module, mem);
}

void testV01() {
    Test test("Parser - v0.1");
    HeapAllocator mem;

    {
        Source *source = io::readFile("data/parser_v_0_1.qak", mem);
        QAK_CHECK(source != nullptr, "Couldn't read test file data/parser_v_0_1.qak");

        Parser parser(mem);
        Errors errors(mem);
        BumpAllocator moduleMem;
        Module *module = parser.parse(*source, errors, &moduleMem);
        if (errors.hasErrors()) errors.print();
        QAK_CHECK(module, "Expected module, got nullptr.");

        {
            HeapAllocator printMem;
            tokenizer::printTokens(parser.tokens(), printMem);
            parser::printAstNode(module, printMem);
        }

        mem.freeObject(source, QAK_SRC_LOC);
    }

    if (mem.numAllocations() != 0) mem.printAllocations();
    QAK_CHECK(mem.numAllocations() == 0, "Expected all memory to be deallocated, but %zu allocations remaining.", mem.numAllocations());
}

int main() {
    testModule();
    testExpression();
    testModuleVariable();
    testFunction();
    testV01();
    testBench();
    return 0;
}