#include "parser.h"

using namespace qak;

using namespace qak::ast;

Module *Parser::parse(Source &source, Errors &errors) {
    _source = &source;
    _errors = &errors;

    _tokens.clear();
    tokenizer::tokenize(source, _tokens, errors);
    if (_errors->hasErrors()) return nullptr;

    TokenStream stream(source, _tokens, errors);
    _stream = &stream;

    Module *module = parseModule();
    if (!module) return nullptr;

    while (_stream->hasMore()) {
        if (_stream->match(QAK_STRING_WITH_LEN("fun"), false)) {
            Function *function = parseFunction();
            if (!function) return nullptr;

            module->functions.add(function);
        } else {
            Statement *statement = parseStatement();
            if (!statement) return nullptr;

            if (statement->astType == AstVariable) module->variables.add(static_cast<Variable *>(statement));
            module->statements.add(statement);
        }
    }
    return module;
}

Module *Parser::parseModule() {
    Token *moduleKeyword = _stream->expect(QAK_STRING_WITH_LEN("module"));
    if (!moduleKeyword) return nullptr;

    Token *moduleName = _stream->expect(Identifier);
    if (!moduleName) return nullptr;

    Module *module = _mem.allocObject<Module>(__FILE__, __LINE__, _mem, moduleName->span);
    return module;
}

Function *Parser::parseFunction() {
    _stream->expect(QAK_STRING_WITH_LEN("fun"));

    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    Array<Parameter *> *parameters = obtainParametersArray();
    if (!parseParameters(*parameters)) return nullptr;

    TypeSpecifier *returnType = nullptr;
    if (_stream->match(QAK_STRING_WITH_LEN(":"), true)) {
        returnType = parseTypeSpecifier();
        if (!returnType) return nullptr;
    }

    Array<Statement *> *statements = obtainStatementArray();
    while (_stream->hasMore() && !_stream->match(QAK_STRING_WITH_LEN("end"), false)) {
        Statement *statement = parseStatement();
        if (!statement) return nullptr;
        statements->add(statement);
    }

    if (!_stream->expect(QAK_STRING_WITH_LEN("end"))) return nullptr;

    Function *function = _bumpMem.allocObject<Function>(_bumpMem, name->span, *parameters, returnType, *statements);
    freeStatementArray(statements);
    freeParameterArray(parameters);
    return function;
}

bool Parser::parseParameters(Array<Parameter *> &parameters) {
    if (!_stream->expect(QAK_STRING_WITH_LEN("("))) return false;

    while (_stream->match(Identifier, false)) {
        Parameter *parameter = parseParameter();
        if (!parameter) return false;

        parameters.add(parameter);

        if (!_stream->match(QAK_STRING_WITH_LEN(","), true)) break;
    }

    return _stream->expect(QAK_STRING_WITH_LEN(")"));
}

ast::Parameter *Parser::parseParameter() {
    Token *name = _stream->consume();
    if (!_stream->expect(QAK_STRING_WITH_LEN(":"))) return nullptr;
    TypeSpecifier *type = parseTypeSpecifier();
    if (!type) return nullptr;

    Parameter *parameter = _bumpMem.allocObject<Parameter>(name->span, type);
    return parameter;
}

Statement *Parser::parseStatement() {
    if (_stream->match(QAK_STRING_WITH_LEN("var"), false)) {
        Variable *variable = parseVariable();
        if (!variable) return nullptr;
        return variable;
    } else {
        Expression *expression = parseExpression();
        if (!expression) return nullptr;
        return expression;
    }
}

Variable *Parser::parseVariable() {
    _stream->expect(QAK_STRING_WITH_LEN("var"));

    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    TypeSpecifier *type = nullptr;
    if (_stream->match(QAK_STRING_WITH_LEN(":"), true)) {
        type = parseTypeSpecifier();
        if (!type) return nullptr;
    }

    Expression *expression = nullptr;
    if (_stream->match(QAK_STRING_WITH_LEN("="), true)) {
        expression = parseExpression();
        if (!expression) return nullptr;
    }

    Variable *variable = _bumpMem.allocObject<Variable>(name->span, type, expression);
    return variable;
}

TypeSpecifier *Parser::parseTypeSpecifier() {
    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    TypeSpecifier *type = _bumpMem.allocObject<TypeSpecifier>(name->span);
    return type;
}

Expression *Parser::parseExpression() {
    return parseTernaryOperator();
}

Expression *Parser::parseTernaryOperator() {
    Expression *condition = parseBinaryOperator(0);
    if (!condition) return nullptr;

    if (_stream->match(QAK_STRING_WITH_LEN("?"), true)) {
        Expression *trueValue = parseTernaryOperator();
        if (!trueValue) return nullptr;
        if (!_stream->match(QAK_STRING_WITH_LEN(":"), true)) return nullptr;
        Expression *falseValue = parseTernaryOperator();
        if (!falseValue) return nullptr;
        TernaryOperation *ternary = _bumpMem.allocObject<TernaryOperation>(condition, trueValue, falseValue);
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

Expression *Parser::parseBinaryOperator(uint32_t level) {
    int nextLevel = level + 1;

    Expression *left = nextLevel == OPERATOR_NUM_GROUPS ? parseUnaryOperator() : parseBinaryOperator(nextLevel);
    if (!left) return nullptr;

    while (_stream->hasMore()) {
        TokenType *op = binaryOperators[level];
        while (*op != OPERATOR_END) {
            if (_stream->match(*op, false)) break;
            op++;
        }
        if (*op == OPERATOR_END) break;

        Token *opToken = _stream->consume();
        Expression *right = nextLevel == OPERATOR_NUM_GROUPS ? parseUnaryOperator() : parseBinaryOperator(nextLevel);
        if (right == nullptr) return nullptr;

        left = _bumpMem.allocObject<BinaryOperation>(opToken->span, left, right);
    }
    return left;
}

static TokenType unaryOperators[] = {Not, Plus, Minus, OPERATOR_END};

Expression *Parser::parseUnaryOperator() {
    TokenType *op = unaryOperators;
    while (*op != OPERATOR_END) {
        if (_stream->match(*op, false)) break;
        op++;
    }
    if (*op != OPERATOR_END) {
        Token *op = _stream->consume();
        Expression *expression = parseUnaryOperator();
        if (!expression) return nullptr;
        UnaryOperation *operation = _bumpMem.allocObject<UnaryOperation>(op->span, expression);
        return operation;
    } else {
        if (_stream->match(QAK_STRING_WITH_LEN("("), true)) {
            Expression *expression = parseExpression();
            if (!expression) return nullptr;
            if (!_stream->expect(QAK_STRING_WITH_LEN(")"))) return nullptr;
            return expression;
        } else {
            return parseAccessOrCallOrLiteral();
        }
    }
    return nullptr;
}

Expression *Parser::parseAccessOrCallOrLiteral() {
    if (_stream->match(StringLiteral, false) ||
        _stream->match(BooleanLiteral, false) ||
        _stream->match(DoubleLiteral, false) ||
        _stream->match(FloatLiteral, false) ||
        _stream->match(ByteLiteral, false) ||
        _stream->match(ShortLiteral, false) ||
        _stream->match(IntegerLiteral, false) ||
        _stream->match(LongLiteral, false) ||
        _stream->match(CharacterLiteral, false) ||
        _stream->match(NullLiteral, false)) {
        Token *token = _stream->consume();
        Literal *literal = _bumpMem.allocObject<Literal>(token->type, token->span);
        return literal;
    } else if (_stream->match(Identifier, false)) {
        return parseAccessOrCall();
    } else {
        _errors->add(_stream->peek()->span, "Expected a variable, field, array, function call, method call, or literal.");
        return nullptr;
    }
}

Expression *Parser::parseAccessOrCall() {
    _errors->add(_stream->peek()->span, "Parsing of variable access or call not implemented.");
    return nullptr;
}
