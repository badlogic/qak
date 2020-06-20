

#ifndef QAK_PARSER_H
#define QAK_PARSER_H

#include "tokenizer.h"

namespace qak {
    namespace ast {
#define QAK_AST_INDENT 3

        struct AstNode {
            Span span;

            AstNode(Span start, Span end) : span(start.source, start.start, end.end) {}

            AstNode(Span span) : span(span) {}

            virtual void print(int indent, HeapAllocator &mem) = 0;
        };

        struct TypeSpecifier : public AstNode {
            Span name;

            TypeSpecifier(Span name) : AstNode(name), name(name) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("type: %s\n", name.toCString(mem));
            }
        };

        struct Expression : public AstNode {
            Expression(Span start, Span end) : AstNode(start, end) {}
        };

        struct TernaryOperation : public Expression {
            Expression *condition;
            Expression *trueValue;
            Expression *falseValue;

            TernaryOperation(Expression *condition, Expression *trueValue, Expression *falseValue) :
                    Expression(condition->span, falseValue->span),
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

            BinaryOperation(Span op, Expression *left, Expression *right) : Expression(left->span, right->span), op(op), left(left), right(right) {}

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

            UnaryOperation(Span op, Expression *expression) : Expression(op, op), op(op), expression(expression) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Unary op: %s\n", op.toCString(mem));
                expression->print(indent + QAK_AST_INDENT, mem);
            }
        };

        struct Variable : public AstNode {
            Span name;
            TypeSpecifier *type;
            Expression *expression;

            Variable(Span name, TypeSpecifier *type, Expression *expression) : AstNode(name), name(name), type(type), expression(expression) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Variable: %s\n", name.toCString(mem));
                if (type) type->print(indent + QAK_AST_INDENT, mem);
                if (expression) {
                    printf("%*s", indent + QAK_AST_INDENT, "");
                    printf("Initializer: \n");
                    expression->print(indent + QAK_AST_INDENT * 2, mem);
                }
            }
        };

        struct Literal : public Expression {
            TokenType type;
            Span value;

            Literal(TokenType type, Span value) : Expression(value, value), type(type), value(value) {}

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("%s: %s\n", tokenizer::tokenTypeToString(type), value.toCString(mem));
            }
        };

        struct Module : public AstNode {
            Span name;
            Array<Variable *> variables;

            Module(Span name, HeapAllocator &mem) : AstNode(name), name(name), variables(mem) {}

            void print(HeapAllocator &mem) {
                print(0, mem);
            }

            void print(int indent, HeapAllocator &mem) {
                printf("%*s", indent, "");
                printf("Module: %s\n", name.toCString(mem));
                for (u8 i = 0; i < variables.getSize(); i++) {
                    variables[i]->print(indent + QAK_AST_INDENT, mem);
                }
            }
        };
    }

    struct Parser {
    private:
        HeapAllocator &mem;
        Source *currentSource;

    public:
        Parser(HeapAllocator &mem) : mem(mem), currentSource(nullptr) {}

        ast::Module *parse(Source source, Errors &errors);

        ast::Module *parseModule(TokenStream &stream, Errors &errors);

        ast::AstNode *parseStatement(TokenStream &stream, ast::Module *module, Errors &errors);

        ast::Variable *parseVariable(TokenStream &stream, Errors &errors);

        ast::TypeSpecifier *parseTypeSpecifier(TokenStream &stream, Errors &errors);

        ast::Expression *parseExpression(TokenStream &stream, Errors &errors);

        ast::Expression *parseTernaryOperator(TokenStream &stream, Errors &errors);

        ast::Expression *parseBinaryOperator(TokenStream &stream, Errors &errors, u4 level);

        ast::Expression *parseUnaryOperator(TokenStream &stream, Errors &errors);

        ast::Expression *parseAccessOrCallOrLiteral(TokenStream &stream, Errors &errors);

        ast::Expression *parseAccessOrCall(TokenStream &stream, Errors &errors);
    };
}


#endif //QAK_PARSER_H
