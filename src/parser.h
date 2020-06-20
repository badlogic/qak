

#ifndef QAK_PARSER_H
#define QAK_PARSER_H

#include "tokenizer.h"

namespace qak {
    struct TokenStream {
        Source source;
        Array<Token> &tokens;
        Errors &errors;
        int index;
        int end;

        TokenStream(Source source, Array<Token> &tokens, Errors &errors) : source(source), tokens(tokens), errors(errors), index(0), end(tokens.getSize()) {}

        /** Returns whether there are more tokens in the stream. **/
        bool hasMore();

        /** Consumes the next token and returns it. **/
        Token *consume();

        /** Returns the current token or null. **/
        Token *peek();

        /** Checks if the next token has the give type and optionally consumes, or throws an error if the next token did not match the
         * type. */
        Token *expect(TokenType type);

        /** Checks if the next token matches the given text and optionally consumes, or throws an error if the next token did not match
         * the text. */
        Token *expect(const char *text);

        /** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
        bool match(TokenType type, bool consume);

        /** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
        bool match(const char *text, bool consume);
    };

    namespace ast {
        struct AstNode {
            Span span;

            AstNode(Span start, Span end) : span(start.source, start.start, end.end) {}

            AstNode(Span span) : span(span) {}
        };

        struct TypeSpecifier : public AstNode {
            Span name;

            TypeSpecifier(Span name) : AstNode(name), name(name) {}
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
        };

        struct BinaryOperation : public Expression {
            Span op;
            Expression *left;
            Expression *right;

            BinaryOperation(Span op, Expression *left, Expression *right) : Expression(left->span, right->span), op(op), left(left), right(right) {}
        };

        struct UnaryOperation : public Expression {
            Span op;
            Expression *expression;

            UnaryOperation(Span op, Expression *expression) : Expression(op, op), op(op), expression(expression) {}
        };

        struct Variable : public AstNode {
            Span name;
            TypeSpecifier *type;
            Expression *expression;

            Variable(Span name, TypeSpecifier *type, Expression *expression) : AstNode(name), name(name), type(type), expression(expression) {}
        };

        struct Literal : public Expression {
            TokenType type;
            Span value;

            Literal(TokenType type, Span value) : Expression(value, value), type(type), value(value) {}
        };

        struct Module : public AstNode {
            Span name;
            Array<Variable *> variables;

            Module(Span name, HeapAllocator &mem) : AstNode(name), name(name), variables(mem) {}
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
