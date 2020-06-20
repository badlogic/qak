#include "parser.h"

using namespace qak;

using namespace qak::ast;

/** Returns whether there are more tokens in the stream. **/
bool TokenStream::hasMore() {
    return index < end;
}

/** Consumes the next token and returns it. **/
Token *TokenStream::consume() {
    if (!hasMore()) return nullptr;
    return &tokens[index++];
}

Token *TokenStream::peek() {
    if (!hasMore()) return nullptr;
    return &tokens[index];
}

/** Checks if the next token has the give type and optionally consumes, or throws an error if the next token did not match the
 * type. */
Token *TokenStream::expect(TokenType type) {
    bool result = match(type, true);
    if (!result) {
        Token *token = (u8) index < tokens.getSize() ? &tokens[index] : nullptr;
        Span *span = token != nullptr ? &token->span : nullptr;
        if (span == nullptr)
            errors.add({source, 0, 0}, "Expected '%s', but reached the end of the source.", tokenizer::tokenTypeToString(type));
        else {
            HeapAllocator mem;
            errors.add(*span, "Expected '%s', but got '%s'", tokenizer::tokenTypeToString(type), token->toCString(mem));
        }
        return nullptr;
    } else {
        return &tokens[index - 1];
    }
}

/** Checks if the next token matches the given text and optionally consumes, or throws an error if the next token did not match
 * the text. */
Token *TokenStream::expect(const char *text) {
    bool result = match(text, true);
    if (!result) {
        Token *token = (u8) index < tokens.getSize() ? &tokens[index] : nullptr;
        Span *span = token != nullptr ? &token->span : nullptr;
        if (span == nullptr) {
            errors.add({source, 0, 0}, "Expected '%s', but reached the end of the source.", text);
        } else {
            HeapAllocator mem;
            errors.add(*span, "Expected '%s', but got '%s'", text, token->toCString(mem));
        }
        return nullptr;
    } else {
        return &tokens[index - 1];
    }
}

/** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
bool TokenStream::match(TokenType type, bool consume) {
    if (index >= end) return false;
    if (tokens[index].type == type) {
        if (consume) index++;
        return true;
    }
    return false;
}

/** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
bool TokenStream::match(const char *text, bool consume) {
    if (index >= end) return false;
    if (tokens[index].match(text)) {
        if (consume) index++;
        return true;
    }
    return false;
}

Module *Parser::parse(Source source, Errors &errors) {
    currentSource = &source;

    Array<Token> tokens(mem);
    tokenizer::tokenize(source, tokens, errors);
    if (errors.hasErrors()) return nullptr;


    TokenStream stream(source, tokens, errors);
    Module *module = parseModule(stream, errors);
    if (!module) return nullptr;

    while (stream.hasMore()) {
        if (!parseStatement(stream, module, errors)) return nullptr;
    }
    return module;
}

Module *Parser::parseModule(TokenStream &stream, Errors &errors) {
    Token *moduleKeyword = stream.expect("module");
    if (!moduleKeyword) return nullptr;

    Token *moduleName = stream.expect(Identifier);
    if (!moduleName) return nullptr;

    Module *module = new(QAK_ALLOC(Module)) Module(moduleName->span, mem);
    return module;
}

AstNode *Parser::parseStatement(TokenStream &stream, Module *module, Errors &errors) {
    if (stream.match("var", false)) {
        Variable *variable = parseVariable(stream, errors);
        if (!variable) return nullptr;
        module->variables.add(variable);
        return variable;
    } else {
        if (stream.hasMore())
            errors.add(stream.consume()->span, "Unknown statement.");
        else
            errors.add({*currentSource, 0, 0}, "Unknown error.");
        return nullptr;
    }
}

Variable *Parser::parseVariable(TokenStream &stream, Errors &errors) {
    stream.expect("var");

    Token *name = stream.expect(Identifier);
    if (!name) return nullptr;

    TypeSpecifier *type = nullptr;
    if (stream.match(":", true)) {
        type = parseTypeSpecifier(stream, errors);
        if (!type) return nullptr;
    }

    Expression *expression = nullptr;
    if (stream.match("=", true)) {
        expression = parseExpression(stream, errors);
        if (!expression) return nullptr;
    }

    Variable *variable = new(QAK_ALLOC(Variable)) Variable(name->span, type, expression);
    return variable;
}

TypeSpecifier *Parser::parseTypeSpecifier(TokenStream &stream, Errors &errors) {
    Token *name = stream.expect(Identifier);
    if (!name) return nullptr;

    TypeSpecifier *type = new(QAK_ALLOC(TypeSpecifier)) TypeSpecifier(name->span);
    return type;
}

Expression *Parser::parseExpression(TokenStream &stream, Errors &errors) {
    return parseTernaryOperator(stream, errors);
}

Expression *Parser::parseTernaryOperator(TokenStream &stream, Errors &errors) {
    Expression *condition = parseBinaryOperator(stream, errors, 0);
    if (!condition) return nullptr;

    if (stream.match("?", true)) {
        Expression *trueValue = parseTernaryOperator(stream, errors);
        if (!trueValue) return nullptr;
        if (!stream.match(":", true)) return nullptr;
        Expression *falseValue = parseTernaryOperator(stream, errors);
        if (!falseValue) return nullptr;
        TernaryOperation *ternary = new(QAK_ALLOC(TernaryOperation)) TernaryOperation(condition, trueValue, falseValue);
        return ternary;
    } else {
        return condition;
    }
}

#define OPERATOR_END (TokenType)0xffff
#define OPERATOR_NUM_GROUPS 6

static TokenType binaryOperators[OPERATOR_NUM_GROUPS][5] = {
        {Assignment, OPERATOR_END},
        {Or,           And,       Xor,        OPERATOR_END},
        {Equal,        NotEqual, OPERATOR_END},
        {Less,         LessEqual, Greater, GreaterEqual, OPERATOR_END},
        {Plus,         Minus,    OPERATOR_END},
        {ForwardSlash, Asterisk,  Percentage, OPERATOR_END}
};

Expression *Parser::parseBinaryOperator(TokenStream &stream, Errors &errors, u4 level) {
    int nextLevel = level + 1;

    Expression *left = nextLevel == OPERATOR_NUM_GROUPS ? parseUnaryOperator(stream, errors) : parseBinaryOperator(stream, errors, nextLevel);
    if (!left) return nullptr;

    TokenType *op = binaryOperators[level];
    while (stream.hasMore()) {
        while (*op != OPERATOR_END) {
            if (stream.match(*op, false)) break;
            op++;
        }
        if (*op == OPERATOR_END) break;

        Token *opToken = stream.consume();
        Expression *right = nextLevel == OPERATOR_NUM_GROUPS ? parseUnaryOperator(stream, errors) : parseBinaryOperator(stream, errors, nextLevel);
        if (right == nullptr) return nullptr;

        left = new(QAK_ALLOC(BinaryOperation)) BinaryOperation(opToken->span, left, right);
    }
    return left;
}

static TokenType unaryOperators[] = {Not, Plus, Minus, OPERATOR_END};

Expression *Parser::parseUnaryOperator(TokenStream &stream, Errors &errors) {
    TokenType *op = unaryOperators;
    while (*op != OPERATOR_END) {
        if (stream.match(*op, false)) break;
        op++;
    }
    if (*op != OPERATOR_END) {
        Token* op = stream.consume();
        Expression* expression = parseUnaryOperator(stream, errors);
        if (!expression) return nullptr;
        UnaryOperation* operation = new (QAK_ALLOC(UnaryOperation)) UnaryOperation(op->span, expression);
        return operation;
    } else {
        if(stream.match("(", true)) {
            Expression* expression = parseExpression(stream, errors);
            if (!expression) return nullptr;
            if(!stream.expect(")")) return nullptr;
            return expression;
        } else {
            return parseAccessOrCallOrLiteral(stream, errors);
        }
    }
    return nullptr;
}

Expression *Parser::parseAccessOrCallOrLiteral(TokenStream &stream, Errors &errors) {
    if (stream.match(StringLiteral, false) ||
        stream.match(BooleanLiteral, false) ||
        stream.match(DoubleLiteral, false) ||
        stream.match(FloatLiteral, false) ||
        stream.match(ByteLiteral, false) ||
        stream.match(ShortLiteral, false) ||
        stream.match(IntegerLiteral, false) ||
        stream.match(LongLiteral, false) ||
        stream.match(CharacterLiteral, false) ||
        stream.match(NullLiteral, false)) {
        Token *token = stream.consume();
        Literal* literal = new (QAK_ALLOC(Literal)) Literal(token->type, token->span);
        return literal;
    } else if (stream.match(Identifier, false)) {
        return parseAccessOrCall(stream, errors);
    } else {
        errors.add(stream.peek()->span, "Expected a variable, field, array, function call, method call, or literal.");
        return nullptr;
    }
}

Expression *Parser::parseAccessOrCall(TokenStream &stream, Errors &errors) {
    errors.add(stream.peek()->span, "Parsing of variable access or call not implemented.");
    return nullptr;
}
