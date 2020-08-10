#include "c/qak.h"
#include "c/io.h"
#include "c/parser.h"
#include "c/tokenizer.h"
#include "test.h"
#include <string.h>

void testInitShutdown() {
    printf("========= Test: parser init/shutdown\n");

    qak_allocator mem;
    qak_allocator_init(&mem);

    qak_parser parser;
    qak_parser_init(&mem, &parser);
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem), "Parser init didn't allocate any  memory.");

    qak_parser_shutdown(&parser);

    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testModule() {
    printf("========= Test: parser simple module\n");
    qak_allocator mem;
    qak_allocator_init(&mem);

    qak_source* source = qak_io_read_source_from_file(&mem, "data/parser_module.qak");
    QAK_CHECK(source, "Couldn't read test file data/parser_module.qak");

    qak_array_error *errors = qak_array_error_new(&mem, 16);
    qak_parser parser;
    qak_parser_init(&mem, &parser);
    qak_ast_node *module = qak_parse(&parser, source, errors);
    QAK_CHECK(module, "Expected module, got null pointer.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testErrorExpect() {
    printf("========= Test: parser test error expect\n");
    qak_allocator mem;
    qak_allocator_init(&mem);

    qak_array_error *errors = qak_array_error_new(&mem, 16);
    qak_parser parser;
    qak_parser_init(&mem, &parser);

    qak_source* source = qak_io_read_source_from_memory(&mem, "error.qak", "123");
    qak_ast_node *module = qak_parse(&parser, source, errors);
    QAK_CHECK(errors->size == 1, "Expected an error, got no error.");
    qak_error_print(source, &errors->items[0]);

    qak_array_error_clear(errors);
    source = qak_io_read_source_from_memory(&mem, "error.qak", "module");
    module = qak_parse(&parser, source, errors);
    QAK_CHECK(errors->size == 1, "Expected an error, got no error.");
    qak_error_print(source, &errors->items[0]);

    qak_array_error_clear(errors);
    source = qak_io_read_source_from_memory(&mem, "error.qak", "  \n\t\n");
    module = qak_parse(&parser, source, errors);
    QAK_CHECK(errors->size == 1, "Expected an error, got no error.");
    qak_error_print(source, &errors->items[0]);

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

int main(int argc, char** argv) {
    QAK_UNUSED(argc);
    QAK_UNUSED(argv);

    testInitShutdown();
    testErrorExpect();
    testModule();
    return 0;
}
