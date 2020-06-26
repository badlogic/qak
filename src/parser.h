

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

            void print(int indent) {
                printf("%*s", indent, "");
                printf("type: %s\n", name.toCString(mem));
            }
        };

        struct Statement : public AstNode {
            Statement(HeapAllocator &mem, AstType astType, Span start, Span end) : AstNode(mem, astType, start, end) {}
        };

        struct Parameter : public AstNode {
            Span name;
            TypeSpecifier *typeSpecifier;

            Parameter(HeapAllocator &mem, Span name, TypeSpecifier *typeSpecifier) :
                    AstNode(mem, AstParameter, name, typeSpecifier->span),
                    name(name),
                    typeSpecifier(typeSpecifier) {
            }

            void print(int indent) {
                printf("%*s", indent, "");
                printf("Parameter: %s\n", name.toCString(mem));
                typeSpecifier->print(indent + QAK_AST_INDENT);
            }
        };

        struct Function : public AstNode {
            Span name;
            FixedArray<Parameter *> parameters;
            TypeSpecifier *returnType;
            FixedArray<Statement *> statements;

            Function(BumpAllocator &bumpMem, HeapAllocator &mem, Span name, Array<Parameter *> &parameters, TypeSpecifier *returnType, Array<Statement *> &statements) :
                    AstNode(mem, AstFunction, name, name), name(name), parameters(bumpMem, parameters), returnType(returnType), statements(bumpMem, statements) {
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
        };

        struct Variable : public Statement {
            Span name;
            TypeSpecifier *typeSpecifier;
            Expression *expression;

            Variable(HeapAllocator &mem, Span name, TypeSpecifier *type, Expression *expression) :
                    Statement(mem, AstVariable, name, name), name(name), typeSpecifier(type), expression(expression) {}

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
            FixedArray<Statement *> statements;

            While(BumpAllocator &bumpMem, HeapAllocator &mem, Span start, Span end, Expression *condition, Array<Statement *> &statements) :
                    Statement(mem, AstWhile, start, end),
                    condition(condition),
                    statements(bumpMem, statements) {
            }

            // BOZO print
        };

        struct If : public Statement {
            Expression *condition;
            FixedArray<Statement *> trueBlock;
            FixedArray<Statement *> falseBlock;

            If(BumpAllocator &bumpMem, HeapAllocator &mem, Span start, Span end, Expression *condition, Array<Statement *> &trueBlock, Array<Statement *> &falseBlock) :
                    Statement(mem, AstIf, start, end),
                    condition(condition),
                    trueBlock(bumpMem, trueBlock),
                    falseBlock(bumpMem, falseBlock) {
            }

            // BOZO print
        };

        struct Return : public Statement {
            Expression *returnValue;

            Return(HeapAllocator &mem, Span start, Span end, Expression *returnValue) : Statement(mem, AstReturn, start, end), returnValue(returnValue) {}

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

            virtual ~Module() { }

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
        BumpAllocator &_bumpMem;
        HeapAllocator &_mem;
        Source *_source;
        TokenStream *_stream;
        Array<Token> _tokens;
        Errors *_errors;
        Array<Array<ast::Statement *>*> _statementArrayPool;
        Array<Array<ast::Parameter *>*> _parameterArrayPool;

    public:
        Parser(BumpAllocator &bumpMem, HeapAllocator &mem) :
        _bumpMem(bumpMem), _mem(mem),
        _source(nullptr),
        _stream(nullptr), _tokens(mem),
        _errors(nullptr),
        _statementArrayPool(mem),
        _parameterArrayPool(mem) {}

        ~Parser() {
            _statementArrayPool.freeObjects();
            _parameterArrayPool.freeObjects();
        }

        ast::Module *parse(Source &source, Errors &errors);

    private:

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

        Array<ast::Statement *> *obtainStatementArray() {
            if (_statementArrayPool.size() == 0) {
                return _mem.allocObject<Array<ast::Statement *>>(__FILE__, __LINE__, _mem);
            } else {
                Array<ast::Statement*> *array = _statementArrayPool[_statementArrayPool.size() - 1];
                _statementArrayPool.removeAt(_statementArrayPool.size() - 1);
                return array;
            }
        }

        void freeStatementArray(Array<ast::Statement *> *array) {
            array->clear();
            _statementArrayPool.add(array);
        }

        Array<ast::Parameter *> *obtainParametersArray() {
            if (_parameterArrayPool.size() == 0) {
                return _mem.allocObject<Array<ast::Parameter *>>(__FILE__, __LINE__, _mem);
            } else {
                Array<ast::Parameter*> *array = _parameterArrayPool[_parameterArrayPool.size() - 1];
                _parameterArrayPool.removeAt(_parameterArrayPool.size() - 1);
                return array;
            }
        }

        void freeParameterArray(Array<ast::Parameter *> *array) {
            array->clear();
            _parameterArrayPool.add(array);
        }
    };
}


#endif //QAK_PARSER_H
