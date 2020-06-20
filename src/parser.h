

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
            AstType astType;
            Span span;

            AstNode(AstType astType, Span start, Span end) : astType(astType), span(start.source, start.start, end.end) {}

            AstNode(AstType astType, Span span) : astType(astType), span(span) {}

            virtual void print(int indent, HeapAllocator &mem) = 0;
        };

        struct TypeSpecifier : public AstNode {
            Span name;

            TypeSpecifier(Span name) : AstNode(AstTypeSpecifier, name), name(name) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("type: %s\n", name.toCString(mem));
            }
        };

        struct Statement : public AstNode {
            Statement(AstType astType, Span start, Span end) : AstNode(astType, start, end) {}
        };

        struct Parameter : public AstNode {
            Span name;
            TypeSpecifier *typeSpecifier;

            Parameter(Span name, TypeSpecifier *typeSpecifier) :
                    AstNode(AstParameter, name, typeSpecifier->span),
                    name(name),
                    typeSpecifier(typeSpecifier) {
            }

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Parameter: %s\n", name.toCString(mem));
                typeSpecifier->print(indent + QAK_AST_INDENT, mem);
            }
        };

        struct Function : public AstNode {
            Span name;
            Array<Parameter *> parameters;
            TypeSpecifier *returnType;
            Array<Statement *> statements;

            Function(Span name, Array<Parameter *> &parameters, TypeSpecifier *returnType, Array<Statement *> &statements, HeapAllocator &mem) :
                    AstNode(AstFunction, name, name), name(name), parameters(mem), returnType(returnType), statements(mem) {
                this->parameters.addAll(parameters);
                this->statements.addAll(statements);
            }

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Function: %s\n", name.toCString(mem));
                if (parameters.size() > 0) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Parameters:\n");
                    for (u8 i = 0; i < parameters.size(); i++)
                        parameters[i]->print(indent + QAK_AST_INDENT * 2, mem);
                }
                if (returnType) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Return type:\n");
                    returnType->print(indent + QAK_AST_INDENT * 2, mem);
                }

                if (statements.size() > 0) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Statements:\n");
                    for (u8 i = 0; i < statements.size(); i++) {
                        statements[i]->print(indent + QAK_AST_INDENT * 2, mem);
                    }
                }
            }
        };

        struct Expression : public Statement {
            Expression(AstType astType, Span start, Span end) : Statement(astType, start, end) {}
        };

        struct Variable : public Statement {
            Span name;
            TypeSpecifier *typeSpecifier;
            Expression *expression;

            Variable(Span name, TypeSpecifier *type, Expression *expression) :
                    Statement(AstVariable, name, name), name(name), typeSpecifier(type), expression(expression) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Variable: %s\n", name.toCString(mem));
                if (typeSpecifier) typeSpecifier->print(indent + QAK_AST_INDENT, mem);
                if (expression) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Initializer: \n");
                    expression->print(indent + QAK_AST_INDENT * 2, mem);
                }
            }
        };

        struct While : public Statement {
            Expression *condition;
            Array<Statement *> statements;

            While(Span start, Span end, Expression *condition, Array<Statement *> &statements, HeapAllocator &mem) :
                    Statement(AstWhile, start, end),
                    condition(condition),
                    statements(mem) {
                this->statements.addAll(statements);
            }
        };

        struct If : public Statement {
            Expression *condition;
            Array<Statement *> trueBlock;
            Array<Statement *> falseBlock;

            If(Span start, Span end, Expression *condition, Array<Statement *> &trueBlock, Array<Statement *> &falseBlock, HeapAllocator &mem) :
                    Statement(AstIf, start, end),
                    condition(condition),
                    trueBlock(mem),
                    falseBlock(mem) {
                this->trueBlock.addAll(trueBlock);
                this->falseBlock.addAll(falseBlock);
            }
        };

        struct Return : public Statement {
            Expression *returnValue;

            Return(Span start, Span end, Expression *returnValue) : Statement(AstReturn, start, end), returnValue(returnValue) {}
        };

        struct TernaryOperation : public Expression {
            Expression *condition;
            Expression *trueValue;
            Expression *falseValue;

            TernaryOperation(Expression *condition, Expression *trueValue, Expression *falseValue) :
                    Expression(AstTernaryOperation, condition->span, falseValue->span),
                    condition(condition),
                    trueValue(trueValue),
                    falseValue(falseValue) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Ternary op:\n");
                condition->print(indent + QAK_AST_INDENT, mem);
                trueValue->print(indent + QAK_AST_INDENT, mem);
                falseValue->print(indent + QAK_AST_INDENT, mem);
            }
        };

        struct BinaryOperation : public Expression {
            Span op;
            Expression *left;
            Expression *right;

            BinaryOperation(Span op, Expression *left, Expression *right) : Expression(AstBinaryOperation, left->span, right->span), op(op), left(left),
                                                                            right(right) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Binary op: %s\n", op.toCString(mem));
                left->print(indent + QAK_AST_INDENT, mem);
                right->print(indent + QAK_AST_INDENT, mem);
            }
        };

        struct UnaryOperation : public Expression {
            Span op;
            Expression *expression;

            UnaryOperation(Span op, Expression *expression) : Expression(AstUnaryOperation, op, op), op(op), expression(expression) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Unary op: %s\n", op.toCString(mem));
                expression->print(indent + QAK_AST_INDENT, mem);
            }
        };

        struct Literal : public Expression {
            TokenType type;
            Span value;

            Literal(TokenType type, Span value) : Expression(AstLiteral, value, value), type(type), value(value) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("%s: %s\n", tokenizer::tokenTypeToString(type), value.toCString(mem));
            }
        };

        struct Module : public AstNode {
            Span name;
            Array<Variable *> variables;
            Array<Function *> functions;
            Array<Statement *> statements;

            Module(Span name, HeapAllocator &mem) :
                    AstNode(AstModule, name),
                    name(name),
                    variables(mem),
                    functions(mem),
                    statements(mem) {
            }

            void print(HeapAllocator &mem) {
                print(0, mem);
            }

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Module: %s\n", name.toCString(mem));

                if (statements.size() > 0) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Module-level statements:\n");
                    for (u8 i = 0; i < statements.size(); i++) {
                        statements[i]->print(indent + QAK_AST_INDENT * 2, mem);
                    }
                }

                for (u8 i = 0; i < functions.size(); i++) {
                    functions[i]->print(indent + QAK_AST_INDENT, mem);
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
