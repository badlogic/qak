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
        if (_stream->match(QAK_STR("fun"), false)) {
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
    Token *moduleKeyword = _stream->expect(QAK_STR("module"));
    if (!moduleKeyword) return nullptr;

    Token *moduleName = _stream->expect(Identifier);
    if (!moduleName) return nullptr;

    Module *module = _mem.allocObject<Module>(__FILE__, __LINE__, _mem, moduleName->span);
    return module;
}

Function *Parser::parseFunction() {
    _stream->expect(QAK_STR("fun"));

    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    Array<Parameter *> *parameters = obtainParametersArray();
    if (!parseParameters(*parameters)) return nullptr;

    TypeSpecifier *returnType = nullptr;
    if (_stream->match(QAK_STR(":"), true)) {
        returnType = parseTypeSpecifier();
        if (!returnType) return nullptr;
    }

    Array<Statement *> *statements = obtainStatementArray();
    while (_stream->hasMore() && !_stream->match(QAK_STR("end"), false)) {
        Statement *statement = parseStatement();
        if (!statement) return nullptr;
        statements->add(statement);
    }

    if (!_stream->expect(QAK_STR("end"))) return nullptr;

    Function *function = _bumpMem.allocObject<Function>(_bumpMem, name->span, *parameters, returnType, *statements);
    freeStatementArray(statements);
    freeParameterArray(parameters);
    return function;
}

bool Parser::parseParameters(Array<Parameter *> &parameters) {
    if (!_stream->expect(QAK_STR("("))) return false;

    while (_stream->match(Identifier, false)) {
        Parameter *parameter = parseParameter();
        if (!parameter) return false;

        parameters.add(parameter);

        if (!_stream->match(QAK_STR(","), true)) break;
    }

    return _stream->expect(QAK_STR(")"));
}

ast::Parameter *Parser::parseParameter() {
    Token *name = _stream->consume();
    if (!_stream->expect(QAK_STR(":"))) return nullptr;
    TypeSpecifier *type = parseTypeSpecifier();
    if (!type) return nullptr;

    Parameter *parameter = _bumpMem.allocObject<Parameter>(name->span, type);
    return parameter;
}

Statement *Parser::parseStatement() {
    if (_stream->match(QAK_STR("var"), false)) {
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
    _stream->expect(QAK_STR("var"));

    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    TypeSpecifier *type = nullptr;
    if (_stream->match(QAK_STR(":"), true)) {
        type = parseTypeSpecifier();
        if (!type) return nullptr;
    }

    Expression *expression = nullptr;
    if (_stream->match(QAK_STR("="), true)) {
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

    if (_stream->match(QAK_STR("?"), true)) {
        Expression *trueValue = parseTernaryOperator();
        if (!trueValue) return nullptr;
        if (!_stream->match(QAK_STR(":"), true)) return nullptr;
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
        if (_stream->match(QAK_STR("("), true)) {
            Expression *expression = parseExpression();
            if (!expression) return nullptr;
            if (!_stream->expect(QAK_STR(")"))) return nullptr;
            return expression;
        } else {
            return parseAccessOrCallOrLiteral();
        }
    }
    return nullptr;
}

Expression *Parser::parseAccessOrCallOrLiteral() {
    if (!_stream->hasMore()) {
        _errors->add(_stream->peek()->span, "Expected a variable, field, array, function call, method call, or literal.");
        return nullptr;
    }

    TokenType tokenType = _stream->peek()->type;

    switch (tokenType) {
        case StringLiteral:
        case BooleanLiteral:
        case DoubleLiteral:
        case FloatLiteral:
        case ByteLiteral:
        case ShortLiteral:
        case IntegerLiteral:
        case LongLiteral:
        case CharacterLiteral:
        case NullLiteral: {
            Token *token = _stream->consume();
            Literal *literal = _bumpMem.allocObject<Literal>(token->type, token->span);
            return literal;
        }

        case Identifier:
            return parseAccessOrCall();
            
        default:
            _errors->add(_stream->peek()->span, "Expected a variable, field, array, function call, method call, or literal.");
            return nullptr;
    }
}

Expression *Parser::parseAccessOrCall() {
    _errors->add(_stream->peek()->span, "Parsing of variable access or call not implemented.");
    return nullptr;
}
