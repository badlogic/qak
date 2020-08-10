#include "c/qak.h"
#include "c/io.h"
#include "c/parser.h"
#include "c/tokenizer.h"
#include "c/error.h"
#include "test.h"
#include <string.h>

void testInitShutdown() {
    printf("========= Test: parser init/shutdown\n");

    qak_allocator mem = qak_heap_allocator_init();
    qak_parser parser = qak_parser_init(&mem);

    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem), "Parser init didn't allocate any  memory.");

    qak_parser_shutdown(&parser);

    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testModule() {
    printf("========= Test: parser simple module\n");
    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_file(&mem, "data/parser_module.qak");
    QAK_CHECK(source, "Couldn't read test file data/parser_module.qak");

    qak_errors errors = qak_errors_init(&mem);
    qak_parser parser = qak_parser_init(&mem);
    qak_ast_node *module = qak_parse(&parser, source, &errors);
    QAK_CHECK(module, "Expected module, got null pointer.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testFunction() {
    printf("========= Test: parser simple module\n");
    qak_allocator mem = qak_heap_allocator_init();

    qak_source *source = qak_io_read_source_from_memory(&mem, "function.qak", "module test\nfunction foo()\nend");
    qak_errors errors = qak_errors_init(&mem);
    qak_parser parser = qak_parser_init(&mem);
    qak_ast_node *module = qak_parse(&parser, source, &errors);
    QAK_CHECK(module, "Expected module, got null pointer.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

void testError() {
    printf("========= Test: parser test error expect\n");
    qak_allocator mem = qak_heap_allocator_init();
    qak_parser parser = qak_parser_init(&mem);

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "");
        qak_parse(&parser, source, &errors);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_source_delete(source);
    }

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "  \n\t\n");
        qak_parse(&parser, source, &errors);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_source_delete(source);
    }

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "123");
        qak_parse(&parser, source, &errors);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_source_delete(source);
    }

    {
        qak_errors errors = qak_errors_init(&mem);
        qak_source *source = qak_io_read_source_from_memory(&mem, "error.qak", "module");
        qak_parse(&parser, source, &errors);
        QAK_CHECK(errors.errors->size == 1, "Expected an error, got no error.");
        qak_error_print(&errors.errors->items[0]);
        qak_errors_shutdown(&errors);
        qak_source_delete(source);
    }

    qak_parser_shutdown(&parser);
    QAK_CHECK(qak_allocator_num_allocated_bytes(&mem) == 0, "Parser shutdown didn't deallocate all memory.");

    qak_allocator_shutdown(&mem);
    printf("SUCCESS\n");
}

int main(int argc, char **argv) {
    QAK_UNUSED(argc);
    QAK_UNUSED(argv);

    /*testInitShutdown();
    testError();
    testModule();*/
    testFunction();
    return 0;
}
