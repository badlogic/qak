#include "c/qak.h"
#include "io.h"
#include "parser.h"

#ifdef WASM
#include <emscripten/emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

using namespace qak;

static void spanToQakSpan(Span &span, qak_span &qakSpan) {
    qakSpan.data.data = (char*)span.source.data + span.start;
    qakSpan.data.length = span.end - span.start;
    qakSpan.start = span.start;
    qakSpan.end = span.end;
    qakSpan.startLine = span.startLine;
    qakSpan.endLine = span.endLine;
}

/** Keeps track of global memory allocated for Sources and Modules via a HeapAllocator **/
struct Compiler {
    HeapAllocator *mem;

    Compiler(HeapAllocator *mem) : mem(mem) {};
};

/** Keeps track of results from all compilation stages for a module. The module itself
 * is memory managed by a Compiler. The data held by the module is memory managed by
 * a BumpAllocator owned by the module. The module has ownership of the Source it was
 * compiled from and frees it upon desctruction. */
struct Module {
    HeapAllocator &mem;
    BumpAllocator *bumpMem;
    Source *source;
    Array<Token> tokens;
    ast::Module *astModule;
    Array<qak_ast_node> *astNodes;
    Errors errors;

    Module(HeapAllocator &mem, BumpAllocator *bumpMem, Source *source, Array<Token> &tokens, ast::Module *astModule, Errors &errors) :
            mem(mem), bumpMem(bumpMem),
            source(source),
            tokens(mem),
            astModule(astModule),
            astNodes(nullptr),
            errors(mem, *bumpMem) {
        this->tokens.addAll(tokens);
        this->errors.addAll(errors);
    };

    ~Module() {
        // Free all objects allocated through the bump allocator such
        // as the AST.
        if (bumpMem) mem.freeObject(bumpMem, QAK_SRC_LOC);

        if (astNodes) mem.freeObject(astNodes, QAK_SRC_LOC);

        // Free all objects allocated through the heap allocator such
        // as the source.
        mem.freeObject(source, QAK_SRC_LOC);
    }

    qak_ast_node_index linearizeAst() {
        if (astNodes == nullptr) {
            astNodes = mem.allocObject<Array<qak_ast_node>>(QAK_SRC_LOC, mem);
            return linearizeAst(*astNodes, astModule);
        } else {
            return ((qak_ast_node_index)astNodes->size()) - 1;
        }
    }

    template<typename T>
    void fixedAstNodeArrayToQakAstNodeList(Array<qak_ast_node> &nodes, FixedArray<T *> &array, qak_ast_node_list &list) {
        list.numNodes = (qak_ast_node_index)array.size();
        if (list.numNodes > 0) {
            list.nodes = bumpMem->alloc<qak_ast_node_index>(list.numNodes);
            for (size_t i = 0; i < list.numNodes; i++) {
                list.nodes[i] = linearizeAst(nodes, array[i]);
            }
        }
    }

    qak_ast_node_index linearizeAst(Array<qak_ast_node> &nodes, qak::ast::AstNode *node) {
        if (node == nullptr) return -1;

        qak_ast_node astNode;
        astNode.type = (qak_ast_type) node->astType;
        spanToQakSpan(node->span, astNode.span);

        switch (node->astType) {
            case qak::ast::AstTypeSpecifier: {
                qak::ast::TypeSpecifier *n = (qak::ast::TypeSpecifier *) (node);
                spanToQakSpan(n->name, astNode.data.typeSpecifier.name);
                break;
            }
            case qak::ast::AstParameter: {
                qak::ast::Parameter *n = (qak::ast::Parameter *) (node);
                spanToQakSpan(n->name, astNode.data.parameter.name);
                astNode.data.parameter.typeSpecifier = linearizeAst(nodes, n->typeSpecifier);
                break;
            }
            case qak::ast::AstFunction: {
                qak::ast::Function *n = (qak::ast::Function *) (node);
                spanToQakSpan(n->name, astNode.data.function.name);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->parameters, astNode.data.function.parameters);
                astNode.data.function.returnType = linearizeAst(nodes, n->returnType);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->statements, astNode.data.function.statements);
                break;
            }
            case qak::ast::AstTernaryOperation: {
                qak::ast::TernaryOperation *n = (qak::ast::TernaryOperation *) (node);
                astNode.data.ternaryOperation.condition = linearizeAst(nodes, n->condition);
                astNode.data.ternaryOperation.trueValue = linearizeAst(nodes, n->trueValue);
                astNode.data.ternaryOperation.falseValue = linearizeAst(nodes, n->falseValue);
                break;
            }
            case qak::ast::AstBinaryOperation: {
                qak::ast::BinaryOperation *n = (qak::ast::BinaryOperation *) (node);
                spanToQakSpan(n->op, astNode.data.binaryOperation.op);
                astNode.data.binaryOperation.left = linearizeAst(nodes, n->left);
                astNode.data.binaryOperation.right = linearizeAst(nodes, n->right);
                break;
            }
            case qak::ast::AstUnaryOperation: {
                qak::ast::UnaryOperation *n = (qak::ast::UnaryOperation *) (node);
                spanToQakSpan(n->op, astNode.data.unaryOperation.op);
                astNode.data.unaryOperation.value = linearizeAst(nodes, n->value);
                break;
            }
            case qak::ast::AstLiteral: {
                qak::ast::Literal *n = (qak::ast::Literal *) (node);
                astNode.data.literal.type = (qak_token_type) n->type;
                spanToQakSpan(n->value, astNode.data.literal.value);
                break;
            }
            case qak::ast::AstVariableAccess: {
                qak::ast::VariableAccess *n = (qak::ast::VariableAccess *) (node);
                spanToQakSpan(n->name, astNode.data.variableAccess.name);
                break;
            }
            case qak::ast::AstFunctionCall: {
                qak::ast::FunctionCall *n = (qak::ast::FunctionCall *) (node);
                astNode.data.functionCall.variableAccess = linearizeAst(nodes, n->variableAccess);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->arguments, astNode.data.functionCall.arguments);
                break;
            }
            case qak::ast::AstVariable: {
                qak::ast::Variable *n = (qak::ast::Variable *) (node);
                spanToQakSpan(n->name, astNode.data.variable.name);
                astNode.data.variable.typeSpecifier = linearizeAst(nodes, n->typeSpecifier);
                astNode.data.variable.initializerExpression = linearizeAst(nodes, n->initializerExpression);
                break;
            }
            case qak::ast::AstWhile: {
                qak::ast::While *n = (qak::ast::While *) (node);
                astNode.data.whileNode.condition = linearizeAst(nodes, n->condition);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->statements, astNode.data.whileNode.statements);
                break;
            }
            case qak::ast::AstIf: {
                qak::ast::If *n = (qak::ast::If *) (node);
                astNode.data.ifNode.condition = linearizeAst(nodes, n->condition);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->trueBlock, astNode.data.ifNode.trueBlock);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->falseBlock, astNode.data.ifNode.falseBlock);
                break;
            }
            case qak::ast::AstReturn: {
                qak::ast::Return *n = (qak::ast::Return *) (node);
                astNode.data.returnNode.returnValue = linearizeAst(nodes, n->returnValue);
                break;
            }
            case qak::ast::AstModule: {
                qak::ast::Module *n = (qak::ast::Module *) (node);
                spanToQakSpan(n->name, astNode.data.module.name);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->variables, astNode.data.module.variables);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->functions, astNode.data.module.functions);
                fixedAstNodeArrayToQakAstNodeList(nodes, n->statements, astNode.data.module.statements);
                break;
            }
        }

        nodes.add(astNode);
        return ((qak_ast_node_index)nodes.size()) - 1;
    }
};

EMSCRIPTEN_KEEPALIVE qak_compiler qak_compiler_new() {
    HeapAllocator *mem = new HeapAllocator();
    Compiler *compiler = mem->allocObject<Compiler>(QAK_SRC_LOC, mem);
    return (qak_compiler) compiler;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_delete(qak_compiler compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator *mem = compiler->mem;
    mem->freeObject(compiler, QAK_SRC_LOC);
    mem->printAllocations();
    delete mem;
}

EMSCRIPTEN_KEEPALIVE void qak_compiler_print_memory_usage(qak_compiler compilerHandle) {
    Compiler *compiler = (Compiler *) compilerHandle;
    compiler->mem->printAllocations();
}

qak_module qak_compile(Compiler *compiler, Source *source) {
    BumpAllocator *bumpMem = compiler->mem->allocObject<BumpAllocator>(QAK_SRC_LOC, *compiler->mem);
    Array<Token> tokens(*compiler->mem);
    Errors errors(*compiler->mem, *bumpMem);

    qak::tokenizer::tokenize(*source, tokens, errors);
    if (errors.hasErrors()) {
        return (qak_module) compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, nullptr, errors);;
    }

    qak::Parser parser(*compiler->mem);
    ast::Module *astModule = parser.parse(*source, errors, bumpMem);
    if (astModule == nullptr) {
        return (qak_module) compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, nullptr, errors);;
    }

    return (qak_module) compiler->mem->allocObject<Module>(QAK_SRC_LOC, *compiler->mem, bumpMem, source, tokens, astModule, errors);
}

EMSCRIPTEN_KEEPALIVE qak_module qak_compiler_compile_file(qak_compiler compilerHandle, const char *fileName) {
    Compiler *compiler = (Compiler *) compilerHandle;
    Source *source = io::readFile(fileName, *compiler->mem);
    if (source == nullptr) return nullptr;
    return qak_compile(compiler, source);
}

EMSCRIPTEN_KEEPALIVE qak_module qak_compiler_compile_source(qak_compiler compilerHandle, const char *fileName, const char *sourceData) {
    Compiler *compiler = (Compiler *) compilerHandle;
    HeapAllocator &mem = *compiler->mem;

    Source *source = Source::fromMemory(mem, fileName, sourceData);
    if (source == nullptr) return nullptr;

    return qak_compile(compiler, source);
}

EMSCRIPTEN_KEEPALIVE void qak_module_delete(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator &mem = module->mem;
    mem.freeObject(module, QAK_SRC_LOC);
}

EMSCRIPTEN_KEEPALIVE void qak_module_get_source(qak_module moduleHandle, qak_source *source) {
    Module *module = (Module *) moduleHandle;
    source->fileName.data = (char*)module->source->fileName;
    source->fileName.length = strlen(module->source->fileName);
    source->data.data = (char*)module->source->data;
    source->data.length = module->source->size;
}

EMSCRIPTEN_KEEPALIVE int qak_module_get_num_errors(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    return (int) module->errors.getErrors().size();
}

EMSCRIPTEN_KEEPALIVE void qak_module_get_error(qak_module moduleHandle, int errorIndex, qak_error *errorResult) {
    Module *module = (Module *) moduleHandle;
    Error &error = module->errors.getErrors()[errorIndex];
    Span &span = error.span;

    errorResult->errorMessage.data = (char*)error.message;
    errorResult->errorMessage.length = strlen(error.message);
    spanToQakSpan(span, errorResult->span);
}

EMSCRIPTEN_KEEPALIVE int qak_module_get_num_tokens(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    return (int) module->tokens.size();
}

EMSCRIPTEN_KEEPALIVE void qak_module_get_token(qak_module moduleHandle, int tokenIndex, qak_token *tokenResult) {
    Module *module = (Module *) moduleHandle;
    Token &token = module->tokens[tokenIndex];

    tokenResult->type = (qak_token_type) token.type;
    spanToQakSpan(token, tokenResult->span);
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_errors(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    module->errors.print();
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_tokens(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    HeapAllocator mem;
    tokenizer::printTokens(module->tokens, mem);
}

EMSCRIPTEN_KEEPALIVE qak_ast_module *qak_module_get_ast(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    qak_ast_node_index moduleIndex = module->linearizeAst();
    if (moduleIndex >= 0) return &module->astNodes->buffer()[moduleIndex].data.module;
    else return nullptr;
}

EMSCRIPTEN_KEEPALIVE qak_ast_node *qak_module_get_ast_node(qak_module moduleHandle, qak_ast_node_index nodeIndex) {
    Module *module = (Module *) moduleHandle;
    if (nodeIndex < 0) return nullptr;
    module->linearizeAst();
    return &module->astNodes->buffer()[nodeIndex];
}

EMSCRIPTEN_KEEPALIVE void qak_module_print_ast(qak_module moduleHandle) {
    Module *module = (Module *) moduleHandle;
    if (module->astModule) {
        HeapAllocator mem;
        parser::printAstNode(module->astModule, mem);
    }
}

#ifdef WASM
EMSCRIPTEN_KEEPALIVE int main(int argc, char** argv) {
    return 0;
}

#define member_size(type, member) sizeof(((type *)0)->member)

EMSCRIPTEN_KEEPALIVE void qak_print_struct_offsets () {
    printf("qak_string (size=%lu)\n", sizeof(qak_string));
    printf("   data: %lu\n", offsetof(qak_string, data));
    printf("   length: %lu\n", offsetof(qak_string, length));

    printf("qak_source (size=%lu)\n", sizeof(qak_source));
    printf("   data: %lu\n", offsetof(qak_source, data));
    printf("   fileName: %lu\n", offsetof(qak_source, fileName));

    printf("qak_span (size=%lu)\n", sizeof(qak_span));
    printf("   data: %lu\n", offsetof(qak_span, data));
    printf("   start: %lu\n", offsetof(qak_span, start));
    printf("   end: %lu\n", offsetof(qak_span, end));
    printf("   startLine: %lu\n", offsetof(qak_span, startLine));
    printf("   endLine: %lu\n", offsetof(qak_span, endLine));

    printf("qak_line (size=%lu)\n", sizeof(qak_line));
    printf("   source: %lu\n", offsetof(qak_line, source));
    printf("   data: %lu\n", offsetof(qak_line, data));
    printf("   lineNumber: %lu\n", offsetof(qak_line, lineNumber));

    printf("qak_token (size=%lu)\n", sizeof(qak_token));
    printf("   type(=%lu): %lu\n", member_size(qak_token, type), offsetof(qak_token, type));
    printf("   span: %lu\n", offsetof(qak_token, span));

    printf("qak_error (size=%lu)\n", sizeof(qak_error));
    printf("   errorMessage: %lu\n", offsetof(qak_error, errorMessage));
    printf("   span: %lu\n", offsetof(qak_error, span));
}
#endif