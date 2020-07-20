#include "parser.h"

using namespace qak;

using namespace qak::ast;

/** Used to keep track of obtained arrays from
 * an ArrayPool. Frees them when the monitor is
 * destructed. Can keep track of a maximum of
 * 4 arrays. */
template<typename T>
struct ArrayPoolMonitor {
    ArrayPool<T> &pool;
    Array<T> *tracked[4];
    size_t numTracked;

    ArrayPoolMonitor(ArrayPool<T> &pool) : pool(pool), numTracked(0) {}

    ~ArrayPoolMonitor() {
        for (size_t i = 0; i < numTracked; i++) {
            pool.free(tracked[i]);
        }
    }

    QAK_FORCE_INLINE Array<T> *obtain(const char *file, int32_t line) {
        Array<T> *result = pool.obtain(file, line);
        tracked[numTracked++] = result;
        return result;
    }
};

Module *Parser::parse(Source &source, Errors &errors, BumpAllocator *bumpMem) {
    _source = &source;
    _errors = &errors;
    _bumpMem = bumpMem;

    ArrayPoolMonitor<Function *> functionArrays(_functionArrayPool);
    ArrayPoolMonitor<Statement *> statementArrays(_statementArrayPool);
    ArrayPoolMonitor<Variable *> variableArrays(_variableArrayPool);

    _tokens.clear();
    tokenizer::tokenize(source, _tokens, errors);
    if (_errors->hasErrors()) return nullptr;

    TokenStream stream(source, _tokens, errors);
    _stream = &stream;

    Module *module = parseModule();
    if (!module) return nullptr;

    Array<Function *> *functions = functionArrays.obtain(QAK_SRC_LOC);
    Array<Statement *> *statements = statementArrays.obtain(QAK_SRC_LOC);
    Array<Variable *> *variables = variableArrays.obtain(QAK_SRC_LOC);

    while (_stream->hasMore()) {
        if (_stream->match(QAK_STR("fun"), false)) {
            Function *function = parseFunction();
            if (!function) return nullptr;

            functions->add(function);
        } else {
            Statement *statement = parseStatement();
            if (!statement) return nullptr;

            if (statement->astType == AstVariable) variables->add(static_cast<Variable *>(statement));
            statements->add(statement);
        }
    }

    module->variables.set(*variables);
    module->statements.set(*statements);
    module->functions.set(*functions);

    return module;
}

Module *Parser::parseModule() {
    Token *moduleKeyword = _stream->expect(QAK_STR("module"));
    if (!moduleKeyword) return nullptr;

    Token *moduleName = _stream->expect(Identifier);
    if (!moduleName) return nullptr;

    Module *module = _bumpMem->allocObject<Module>(*_bumpMem, *moduleName);
    return module;
}

Function *Parser::parseFunction() {
    _stream->expect(QAK_STR("fun"));

    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    ArrayPoolMonitor<Parameter *> parameterArrays(_parameterArrayPool);
    Array<Parameter *> *parameters = parameterArrays.obtain(QAK_SRC_LOC);
    if (!parseParameters(*parameters)) return nullptr;

    TypeSpecifier *returnType = nullptr;
    if (_stream->match(QAK_STR(":"), true)) {
        returnType = parseTypeSpecifier();
        if (!returnType) return nullptr;
    }

    ArrayPoolMonitor<Statement *> statementArrays(_statementArrayPool);
    Array<Statement *> *statements = statementArrays.obtain(QAK_SRC_LOC);
    while (_stream->hasMore() && !_stream->match(QAK_STR("end"), false)) {
        Statement *statement = parseStatement();
        if (!statement) return nullptr;
        statements->add(statement);
    }

    if (!_stream->expect(QAK_STR("end"))) return nullptr;

    Function *function = _bumpMem->allocObject<Function>(*_bumpMem, *name, *parameters, returnType, *statements);
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

    Parameter *parameter = _bumpMem->allocObject<Parameter>(*name, type);
    return parameter;
}

Statement *Parser::parseStatement() {
    if (_stream->match(QAK_STR("var"), false)) {
        return parseVariable();
    } else if (_stream->match(QAK_STR("while"), false)) {
        return parseWhile();
    } else if (_stream->match(QAK_STR("if"), false)) {
        return parseIf();
    } else if (_stream->match(QAK_STR("return"), false)) {
        return parseReturn();
    } else {
        return parseExpression();
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

    Variable *variable = _bumpMem->allocObject<Variable>(*name, type, expression);
    return variable;
}

While *Parser::parseWhile() {
    Token *whileToken = _stream->expect(QAK_STR("while"));

    Expression *condition = parseExpression();
    if (!condition) return nullptr;

    ArrayPoolMonitor<Statement *> statementArrays(_statementArrayPool);
    Array<Statement *> *statements = statementArrays.obtain(QAK_SRC_LOC);
    while (_stream->hasMore() && !_stream->match(QAK_STR("end"), false)) {
        Statement *statement = parseStatement();
        if (statement == nullptr) return nullptr;
        statements->add(statement);
    }

    // BOZO expect should also take a custom error string, so we can
    // say something like "Expected a closing 'end' for 'while' statement".
    // Fix this up anywhere else we use expect() as well.
    Token *endToken = _stream->expect(QAK_STR("end"));
    if (!endToken) return nullptr;

    While *whileStmt = _bumpMem->allocObject<While>(*_bumpMem, *whileToken, *endToken, condition, *statements);

    return whileStmt;
}

If *Parser::parseIf() {
    Token *ifToken = _stream->expect(QAK_STR("if"));

    Expression *condition = parseExpression();
    if (!condition) return nullptr;

    ArrayPoolMonitor<Statement *> statementArrays(_statementArrayPool);
    Array<Statement *> *trueBlock = statementArrays.obtain(QAK_SRC_LOC);
    while (_stream->hasMore() && !_stream->match(QAK_STR("end"), false) && !_stream->match(QAK_STR("else"), false)) {
        Statement *statement = parseStatement();
        if (statement == nullptr) return nullptr;
        trueBlock->add(statement);
    }

    Array<Statement *> *falseBlock = statementArrays.obtain(QAK_SRC_LOC);
    if (_stream->match(QAK_STR("else"), true)) {
        while (_stream->hasMore() && !_stream->match(QAK_STR("end"), false)) {
            Statement *statement = parseStatement();
            if (statement == nullptr) return nullptr;
            falseBlock->add(statement);
        }
    }

    Token *endToken = _stream->expect(QAK_STR("end"));
    if (!endToken) return nullptr;

    If *ifStmt = _bumpMem->allocObject<If>(*_bumpMem, *ifToken, *endToken, condition, *trueBlock, *falseBlock);
    return ifStmt;
}

Return *Parser::parseReturn() {
    Token *returnToken = _stream->expect(QAK_STR("return"));

    if (_stream->match(QAK_STR(";"), true)) {
        return _bumpMem->allocObject<Return>(*returnToken, *returnToken, nullptr);
    } else {
        Expression *returnValue = parseExpression();
        if (!returnValue) return nullptr;
        return _bumpMem->allocObject<Return>(*returnToken, returnValue->span, returnValue);
    }
}

TypeSpecifier *Parser::parseTypeSpecifier() {
    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    TypeSpecifier *type = _bumpMem->allocObject<TypeSpecifier>(*name);
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
        TernaryOperation *ternary = _bumpMem->allocObject<TernaryOperation>(condition, trueValue, falseValue);
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

        left = _bumpMem->allocObject<BinaryOperation>(*opToken, left, right);
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
        UnaryOperation *operation = _bumpMem->allocObject<UnaryOperation>(*op, expression);
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
        if (_stream->getTokens().size() > 0) {
            Token token = _stream->getTokens()[_stream->getTokens().size() - 1];
            _errors->add(token, "Expected a variable, field, array, function call, method call, or literal.");
        } else {
            _errors->add(Span(*_source, 0, 0, 0, 0), "Expected a variable, field, array, function call, method call, or literal.");
        }
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
        case NothingLiteral: {
            Token *token = _stream->consume();
            Literal *literal = _bumpMem->allocObject<Literal>(token->type, *token);
            return literal;
        }

        case Identifier:
            return parseAccessOrCall();

        default:
            _errors->add(*_stream->peek(), "Expected a variable, field, array, function call, method call, or literal.");
            return nullptr;
    }
}

Expression *Parser::parseAccessOrCall() {
    Token *name = _stream->expect(Identifier);
    if (!name) return nullptr;

    Expression *result = _bumpMem->allocObject<VariableAccess>(*name);

    // If the next token is "(", we have a function call.
    if (_stream->match(QAK_STR("("), true)) {
        ArrayPoolMonitor<Expression *> expressionArrays(_expressionArrayPool);
        Array<Expression *> *arguments = parseArguments(expressionArrays.obtain(QAK_SRC_LOC));
        if (!arguments) return nullptr;

        Token *closingParan = _stream->expect(QAK_STR(")"));
        if (!closingParan) return nullptr;

        result = _bumpMem->allocObject<FunctionCall>(*_bumpMem, *name, *closingParan, result, *arguments);
    }
    return result;
}


Array<ast::Expression *> *Parser::parseArguments(Array<Expression *> *arguments) {
    while (_stream->hasMore() && !_stream->match(QAK_STR(")"), false)) {
        Expression *argument = parseExpression();
        if (!argument) return nullptr;
        arguments->add(argument);

        if (!_stream->match(QAK_STR(")"), false)) {
            if (!_stream->hasMore()) {
                Token token = _stream->getTokens()[_stream->getTokens().size() - 1];
                _errors->add(token, "Expected ) or , but reached end of file.");
                return nullptr;
            }
            if (!_stream->expect(QAK_STR(","))) return nullptr;
        }
    }

    return arguments;
}

Array<Token> &Parser::tokens() {
    return _tokens;
}

static void printIndent(int indent) {
    printf("%*s", indent, "");
}

static void printAstNodeRecursive(ast::AstNode *node, int indent, HeapAllocator &mem) {
    switch (node->astType) {
        case AstTypeSpecifier: {
            TypeSpecifier *n = (TypeSpecifier *) (node);
            printIndent(indent);
            printf("type: %s\n", n->name.toCString(mem));
            break;
        }
        case AstParameter: {
            Parameter *n = (Parameter *) (node);
            printIndent(indent);
            printf("Parameter: %s\n", n->name.toCString(mem));
            printAstNodeRecursive(n->typeSpecifier, indent + QAK_AST_INDENT, mem);
            break;
        }
        case AstFunction: {
            Function *n = (Function *) (node);
            printIndent(indent);
            printf("Function: %s\n", n->name.toCString(mem));
            if (n->parameters.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Parameters:\n");
                for (size_t i = 0; i < n->parameters.size(); i++)
                    printAstNodeRecursive(n->parameters[i], indent + QAK_AST_INDENT * 2, mem);
            }
            if (n->returnType) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Return type:\n");
                printAstNodeRecursive(n->returnType, indent + QAK_AST_INDENT * 2, mem);
            }

            if (n->statements.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Statements:\n");
                for (size_t i = 0; i < n->statements.size(); i++) {
                    printAstNodeRecursive(n->statements[i], indent + QAK_AST_INDENT * 2, mem);
                }
            }
            break;
        }
        case AstVariable: {
            Variable *n = (Variable *) (node);
            printIndent(indent);
            printf("Variable: %s\n", n->name.toCString(mem));
            if (n->typeSpecifier) printAstNodeRecursive(n->typeSpecifier, indent + QAK_AST_INDENT, mem);
            if (n->expression) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Initializer: \n");
                printAstNodeRecursive(n->expression, indent + QAK_AST_INDENT * 2, mem);
            }
            break;
        }
        case AstWhile: {
            While *n = (While *) (node);
            printIndent(indent);
            printf("While\n");

            printIndent(indent + QAK_AST_INDENT);
            printf("Condition: \n");
            printAstNodeRecursive(n->condition, indent + QAK_AST_INDENT * 2, mem);

            if (n->statements.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Statements: \n");
                for (size_t i = 0; i < n->statements.size(); i++) {
                    printAstNodeRecursive(n->statements[i], indent + QAK_AST_INDENT * 2, mem);
                }
            }
            break;
        }
        case AstIf: {
            If *n = (If *) (node);
            printIndent(indent);
            printf("If\n");

            printIndent(indent + QAK_AST_INDENT);
            printf("Condition: \n");
            printAstNodeRecursive(n->condition, indent + QAK_AST_INDENT * 2, mem);

            if (n->trueBlock.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("True-block statements: \n");
                for (size_t i = 0; i < n->trueBlock.size(); i++) {
                    printAstNodeRecursive(n->trueBlock[i], indent + QAK_AST_INDENT * 2, mem);
                }
            }

            if (n->falseBlock.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("False-block statements: \n");
                for (size_t i = 0; i < n->falseBlock.size(); i++) {
                    printAstNodeRecursive(n->falseBlock[i], indent + QAK_AST_INDENT * 2, mem);
                }
            }
            break;
        }
        case AstReturn: {
            Return *n = (Return *) node;
            printIndent(indent);
            printf("Return:\n");

            if (n->returnValue) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Value:\n");
                printAstNodeRecursive(n->returnValue, indent + QAK_AST_INDENT * 2, mem);
            }

            break;
        }
        case AstTernaryOperation: {
            TernaryOperation *n = (TernaryOperation *) node;
            printIndent(indent);
            printf("Ternary operator:\n");
            printAstNodeRecursive(n->condition, indent + QAK_AST_INDENT, mem);
            printAstNodeRecursive(n->trueValue, indent + QAK_AST_INDENT, mem);
            printAstNodeRecursive(n->falseValue, indent + QAK_AST_INDENT, mem);
            break;
        }
        case AstBinaryOperation: {
            BinaryOperation *n = (BinaryOperation *) node;
            printIndent(indent);
            printf("Binary operator: %s\n", n->op.toCString(mem));
            printAstNodeRecursive(n->left, indent + QAK_AST_INDENT, mem);
            printAstNodeRecursive(n->right, indent + QAK_AST_INDENT, mem);
            break;
        }
        case AstUnaryOperation: {
            UnaryOperation *n = (UnaryOperation *) node;
            printIndent(indent);
            printf("Unary op: %s\n", n->op.toCString(mem));
            printAstNodeRecursive(n->expression, indent + QAK_AST_INDENT, mem);
            break;
        }
        case AstLiteral: {
            Literal *n = (Literal *) node;
            printIndent(indent);
            printf("%s: %s\n", tokenizer::tokenTypeToString(n->type), n->value.toCString(mem));
            break;
        }
        case AstVariableAccess: {
            VariableAccess *n = (VariableAccess *) node;
            printIndent(indent);
            printf("Variable access: %s\n", n->name.toCString(mem));
            break;
        }
        case AstFunctionCall: {
            FunctionCall *n = (FunctionCall *) node;
            printIndent(indent);
            printf("Function call: %s(%s)\n", n->variableAccess->span.toCString(mem), n->arguments.size() > 0 ? "..." : "");

            if (n->arguments.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Arguments:\n");
                for (size_t i = 0; i < n->arguments.size(); i++) {
                    printAstNodeRecursive(n->arguments[i], indent + QAK_AST_INDENT * 2, mem);
                }
            }
            break;
        }
        case AstModule: {
            Module *n = (Module *) node;
            printIndent(indent);
            printf("Module: %s\n", n->name.toCString(mem));

            if (n->statements.size() > 0) {
                printIndent(indent + QAK_AST_INDENT);
                printf("Module statements:\n");
                for (size_t i = 0; i < n->statements.size(); i++) {
                    printAstNodeRecursive(n->statements[i], indent + QAK_AST_INDENT * 2, mem);
                }
            }

            for (size_t i = 0; i < n->functions.size(); i++) {
                printAstNodeRecursive(n->functions[i], indent + QAK_AST_INDENT, mem);
            }
            break;
        }
    }
}

void parser::printAstNode(ast::AstNode *node, HeapAllocator &mem) {
    printAstNodeRecursive(node, 0, mem);
}