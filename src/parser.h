

#ifndef QAK_PARSER_H
#define QAK_PARSER_H

#include "tokenizer.h"

namespace qak {
    namespace ast {
#define QAK_AST_INDENT 3

        enum AstType {
            AstTypeSpecifier,
            AstParameter,
            AstFunction,
            AstTernaryOperation,
            AstBinaryOperation,
            AstUnaryOperation,
            AstLiteral,
            AstVariable,
            AstWhile,
            AstIf,
            AstReturn,
            AstModule
        };

        struct AstNode {
            HeapAllocator &mem;
            AstType astType;
            Span span;

            AstNode(HeapAllocator &mem, AstType astType, Span start, Span end) : mem(mem), astType(astType), span(start.source, start.start, end.end) {}

            AstNode(HeapAllocator &mem, AstType astType, Span span) : mem(mem), astType(astType), span(span) {}

            virtual ~AstNode() {}

            virtual void print(int indent) = 0;
        };

        struct TypeSpecifier : public AstNode {
            Span name;

            TypeSpecifier(HeapAllocator &mem, Span name) : AstNode(mem, AstTypeSpecifier, name), name(name) {}

            virtual ~TypeSpecifier() {}

            void print(int indent) {
                printf("%*s", indent, "");
                printf("type: %s\n", name.toCString(mem));
            }
        };

        struct Statement : public AstNode {
            Statement(HeapAllocator &mem, AstType astType, Span start, Span end) : AstNode(mem, astType, start, end) {}

            virtual ~Statement() {}
        };

        struct Parameter : public AstNode {
            Span name;
            TypeSpecifier *typeSpecifier;

            Parameter(HeapAllocator &mem, Span name, TypeSpecifier *typeSpecifier) :
                    AstNode(mem, AstParameter, name, typeSpecifier->span),
                    name(name),
                    typeSpecifier(typeSpecifier) {
            }

            virtual ~Parameter() {
                mem.freeObject(typeSpecifier, __FILE__, __LINE__);
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Parameter: %s\n", name.toCString(mem));
                typeSpecifier->print(indent + QAK_AST_INDENT);
            }
        };

        struct Function : public AstNode {
            Span name;
            Array<Parameter *> parameters;
            TypeSpecifier *returnType;
            Array<Statement *> statements;

            Function(HeapAllocator &mem, Span name, Array<Parameter *> &parameters, TypeSpecifier *returnType, Array<Statement *> &statements) :
                    AstNode(mem, AstFunction, name, name), name(name), parameters(mem), returnType(returnType), statements(mem) {
                this->parameters.addAll(parameters);
                this->statements.addAll(statements);
            }

            virtual ~Function() {
                parameters.freeObjects();
                if (returnType) mem.freeObject(returnType, __FILE__, __LINE__);
                statements.freeObjects();
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Function: %s\n", name.toCString(mem));
                if (parameters.size() > 0) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Parameters:\n");
                    for (u8 i = 0; i < parameters.size(); i++)
                        parameters[i]->print(indent + QAK_AST_INDENT * 2);
                }
                if (returnType) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Return type:\n");
                    returnType->print(indent + QAK_AST_INDENT * 2);
                }

                if (statements.size() > 0) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Statements:\n");
                    for (u8 i = 0; i < statements.size(); i++) {
                        statements[i]->print(indent + QAK_AST_INDENT * 2);
                    }
                }
            }
        };

        struct Expression : public Statement {
            Expression(HeapAllocator &mem, AstType astType, Span start, Span end) : Statement(mem, astType, start, end) {}

            virtual ~Expression() {}
        };

        struct Variable : public Statement {
            Span name;
            TypeSpecifier *typeSpecifier;
            Expression *expression;

            Variable(HeapAllocator &mem, Span name, TypeSpecifier *type, Expression *expression) :
                    Statement(mem, AstVariable, name, name), name(name), typeSpecifier(type), expression(expression) {}

            virtual ~Variable() {
                if (typeSpecifier) mem.freeObject(typeSpecifier, __FILE__, __LINE__);
                if (expression) mem.freeObject(expression, __FILE__, __LINE__);
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Variable: %s\n", name.toCString(mem));
                if (typeSpecifier) typeSpecifier->print(indent + QAK_AST_INDENT);
                if (expression) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Initializer: \n");
                    expression->print(indent + QAK_AST_INDENT * 2);
                }
            }
        };

        struct While : public Statement {
            Expression *condition;
            Array<Statement *> statements;

            While(HeapAllocator &mem, Span start, Span end, Expression *condition, Array<Statement *> &statements) :
                    Statement(mem, AstWhile, start, end),
                    condition(condition),
                    statements(mem) {
                this->statements.addAll(statements);
            }

            virtual ~While() {
                mem.freeObject(condition, __FILE__, __LINE__);
                statements.freeObjects();
            }

            // BOZO print
        };

        struct If : public Statement {
            Expression *condition;
            Array<Statement *> trueBlock;
            Array<Statement *> falseBlock;

            If(HeapAllocator &mem, Span start, Span end, Expression *condition, Array<Statement *> &trueBlock, Array<Statement *> &falseBlock) :
                    Statement(mem, AstIf, start, end),
                    condition(condition),
                    trueBlock(mem),
                    falseBlock(mem) {
                this->trueBlock.addAll(trueBlock);
                this->falseBlock.addAll(falseBlock);
            }

            virtual ~If() {
                mem.freeObject(condition, __FILE__, __LINE__);
                trueBlock.freeObjects();
                falseBlock.freeObjects();
            }

            // BOZO print
        };

        struct Return : public Statement {
            Expression *returnValue;

            Return(HeapAllocator &mem, Span start, Span end, Expression *returnValue) : Statement(mem, AstReturn, start, end), returnValue(returnValue) {}

            virtual ~Return() {
                if (returnValue) mem.freeObject(returnValue, __FILE__, __LINE__);
            }

            // BOZO print
        };

        struct TernaryOperation : public Expression {
            Expression *condition;
            Expression *trueValue;
            Expression *falseValue;

            TernaryOperation(HeapAllocator &mem, Expression *condition, Expression *trueValue, Expression *falseValue) :
                    Expression(mem, AstTernaryOperation, condition->span, falseValue->span),
                    condition(condition),
                    trueValue(trueValue),
                    falseValue(falseValue) {}

            virtual ~TernaryOperation() {
                mem.freeObject(condition, __FILE__, __LINE__);
                mem.freeObject(trueValue, __FILE__, __LINE__);
                mem.freeObject(falseValue, __FILE__, __LINE__);
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Ternary op:\n");
                condition->print(indent + QAK_AST_INDENT);
                trueValue->print(indent + QAK_AST_INDENT);
                falseValue->print(indent + QAK_AST_INDENT);
            }
        };

        struct BinaryOperation : public Expression {
            Span op;
            Expression *left;
            Expression *right;

            BinaryOperation(HeapAllocator &mem, Span op, Expression *left, Expression *right) : Expression(mem, AstBinaryOperation, left->span, right->span),
                                                                                                op(op), left(left),
                                                                                                right(right) {}

            virtual ~BinaryOperation() {
                mem.freeObject(left, __FILE__, __LINE__);
                mem.freeObject(right, __FILE__, __LINE__);
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Binary op: %s\n", op.toCString(mem));
                left->print(indent + QAK_AST_INDENT);
                right->print(indent + QAK_AST_INDENT);
            }
        };

        struct UnaryOperation : public Expression {
            Span op;
            Expression *expression;

            UnaryOperation(HeapAllocator &mem, Span op, Expression *expression) : Expression(mem, AstUnaryOperation, op, op), op(op), expression(expression) {}

            virtual ~UnaryOperation() {
                mem.freeObject(expression, __FILE__, __LINE__);
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Unary op: %s\n", op.toCString(mem));
                expression->print(indent + QAK_AST_INDENT);
            }
        };

        struct Literal : public Expression {
            TokenType type;
            Span value;

            Literal(HeapAllocator &mem, TokenType type, Span value) : Expression(mem, AstLiteral, value, value), type(type), value(value) {}

            void print(int indent) {
                printf("%*s", indent, "");
                printf("%s: %s\n", tokenizer::tokenTypeToString(type), value.toCString(mem));
            }
        };

        struct Module : public AstNode {
            Span name;
            Array<Variable *> variables;
            Array<Function *> functions;
            Array<Statement *> statements;

            Module(HeapAllocator &mem, Span name) :
                    AstNode(mem, AstModule, name),
                    name(name),
                    variables(mem),
                    functions(mem),
                    statements(mem) {
            }

            virtual ~Module() {
                functions.freeObjects();
                statements.freeObjects();
            }

            void print() {
                print(0);
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Module: %s\n", name.toCString(mem));

                if (statements.size() > 0) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Module-level statements:\n");
                    for (u8 i = 0; i < statements.size(); i++) {
                        statements[i]->print(indent + QAK_AST_INDENT * 2);
                    }
                }

                for (u8 i = 0; i < functions.size(); i++) {
                    functions[i]->print(indent + QAK_AST_INDENT);
                }
            }
        };
    }

    struct Parser {
    private:
        HeapAllocator &_mem;
        Source *_source;
        TokenStream *_stream;
        Errors *_errors;

    public:
        Parser(HeapAllocator &mem) : _mem(mem), _source(nullptr), _stream(nullptr), _errors(nullptr) {}

        ast::Module *parse(Source source, Errors &errors);

        ast::Module *parseModule();

        ast::Function *parseFunction();

        bool parseParameters(Array<ast::Parameter *> &parameters);

        ast::Parameter *parseParameter();

        ast::Statement *parseStatement();

        ast::Variable *parseVariable();

        ast::TypeSpecifier *parseTypeSpecifier();

        ast::Expression *parseExpression();

        ast::Expression *parseTernaryOperator();

        ast::Expression *parseBinaryOperator(u4 level);

        ast::Expression *parseUnaryOperator();

        ast::Expression *parseAccessOrCallOrLiteral();

        ast::Expression *parseAccessOrCall();
    };
}


#endif //QAK_PARSER_H
