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
            AstVariableAccess,
            AstFunctionCall,
            AstVariable,
            AstWhile,
            AstIf,
            AstReturn,
            AstModule
        };

        struct AstNode {
            AstType astType;
            Span span;

            AstNode(AstType astType, Span &start, Span &end) :
                    astType(astType),
                    span(start.source, start.start, start.startLine, end.end, end.endLine) {}

            AstNode(AstType astType, Span &span) : astType(astType), span(span) {}
        };

        struct TypeSpecifier : public AstNode {
            Span name;

            TypeSpecifier(Span name) :
                    AstNode(AstTypeSpecifier, name),
                    name(name) {}
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
                    typeSpecifier(typeSpecifier) {}
        };

        struct Function : public AstNode {
            Span name;
            FixedArray<Parameter *> parameters;
            TypeSpecifier *returnType;
            FixedArray<Statement *> statements;

            Function(BumpAllocator &bumpMem, Span name, Array<Parameter *> &parameters, TypeSpecifier *returnType,
                     Array<Statement *> &statements) :
                    AstNode(AstFunction, name, name),
                    name(name),
                    parameters(bumpMem, parameters),
                    returnType(returnType),
                    statements(bumpMem, statements) {}
        };

        struct Expression : public Statement {
            Expression(AstType astType, Span start, Span end) : Statement(astType, start, end) {}
        };

        struct Variable : public Statement {
            Span name;
            TypeSpecifier *typeSpecifier;
            Expression *expression;

            Variable(Span name, TypeSpecifier *type, Expression *expression) :
                    Statement(AstVariable, name, name),
                    name(name),
                    typeSpecifier(type),
                    expression(expression) {}
        };

        struct While : public Statement {
            Expression *condition;
            FixedArray<Statement *> statements;

            While(BumpAllocator &bumpMem, Span start, Span end, Expression *condition, Array<Statement *> &statements) :
                    Statement(AstWhile, start, end),
                    condition(condition),
                    statements(bumpMem, statements) {}
        };

        struct If : public Statement {
            Expression *condition;
            FixedArray<Statement *> trueBlock;
            FixedArray<Statement *> falseBlock;

            If(BumpAllocator &bumpMem, Span start, Span end, Expression *condition, Array<Statement *> &trueBlock,
               Array<Statement *> &falseBlock) :
                    Statement(AstIf, start, end),
                    condition(condition),
                    trueBlock(bumpMem, trueBlock),
                    falseBlock(bumpMem, falseBlock) {}
        };

        struct Return : public Statement {
            Expression *returnValue;

            Return(Span start, Span end, Expression *returnValue) :
                    Statement(AstReturn, start, end),
                    returnValue(returnValue) {}
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
        };

        struct BinaryOperation : public Expression {
            Span op;
            Expression *left;
            Expression *right;

            BinaryOperation(Span op, Expression *left, Expression *right) :
                    Expression(AstBinaryOperation, left->span, right->span),
                    op(op), left(left),
                    right(right) {}
        };

        struct UnaryOperation : public Expression {
            Span op;
            Expression *expression;

            UnaryOperation(Span op, Expression *expression) :
                    Expression(AstUnaryOperation, op, op),
                    op(op), expression(expression) {}
        };

        struct Literal : public Expression {
            TokenType type;
            Span value;

            Literal(TokenType type, Span value) :
                    Expression(AstLiteral, value, value),
                    type(type),
                    value(value) {}
        };

        struct VariableAccess : public Expression {
            Span name;

            VariableAccess(Span name) :
                    Expression(AstVariableAccess, name, name),
                    name(name) {}
        };

        struct FunctionCall : public Expression {
            Expression *variableAccess;
            FixedArray<Expression *> arguments;

            FunctionCall(BumpAllocator &mem, Span start, Span end, Expression *variableAccess, Array<Expression *> &arguments) :
                    Expression(AstFunctionCall, start, end),
                    variableAccess(variableAccess),
                    arguments(mem, arguments) {}
        };

        struct Module : public AstNode {
            BumpAllocator &mem;
            Span name;
            FixedArray<Variable *> variables;
            FixedArray<Function *> functions;
            FixedArray<Statement *> statements;

            Module(BumpAllocator &mem, Span name) :
                    AstNode(AstModule, name),
                    mem(mem),
                    name(name),
                    variables(mem),
                    functions(mem),
                    statements(mem) {
            }
        };
    }

    class Parser {
    private:
        Array<Token> _tokens;
        ArrayPool<ast::Variable *> _variableArrayPool;
        ArrayPool<ast::Statement *> _statementArrayPool;
        ArrayPool<ast::Function *> _functionArrayPool;
        ArrayPool<ast::Parameter *> _parameterArrayPool;
        ArrayPool<ast::Expression *> _expressionArrayPool;

        // Set on each call to parse.
        Source *_source;
        TokenStream *_stream;
        Errors *_errors;
        BumpAllocator *_bumpMem;

        ast::Module *parseModule();

        ast::Function *parseFunction();

        bool parseParameters(Array<ast::Parameter *> &parameters);

        ast::Parameter *parseParameter();

        ast::Statement *parseStatement();

        ast::Variable *parseVariable();

        ast::While *parseWhile();

        ast::If *parseIf();

        ast::Return *parseReturn();

        ast::TypeSpecifier *parseTypeSpecifier();

        ast::Expression *parseExpression();

        ast::Expression *parseTernaryOperator();

        ast::Expression *parseBinaryOperator(uint32_t level);

        ast::Expression *parseUnaryOperator();

        ast::Expression *parseAccessOrCallOrLiteral();

        ast::Expression *parseAccessOrCall();

        Array<ast::Expression *> *parseArguments(Array<ast::Expression *> *arguments);

    public:
        Parser(HeapAllocator &mem) :
                _tokens(mem),
                _variableArrayPool(mem),
                _statementArrayPool(mem),
                _functionArrayPool(mem),
                _parameterArrayPool(mem),
                _expressionArrayPool(mem),
                _source(nullptr),
                _stream(nullptr),
                _errors(nullptr),
                _bumpMem(nullptr) {}

        ast::Module *parse(Source &source, Errors &errors, BumpAllocator *bumpMem);

        Array<Token> &tokens();
    };

    namespace parser {
        void printAstNode(ast::AstNode *node, HeapAllocator &mem);
    }
}


#endif //QAK_PARSER_H
