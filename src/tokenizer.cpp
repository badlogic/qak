#include "tokenizer.h"

using namespace qak;

static const uint32_t literalToTokenType[] = {
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 25 /* ! */, 27, 27,
        27, 8 /* % */, 22 /* & */, 27, 9 /* ( */, 10 /* ) */, 6 /* * */, 4 /* + */, 1 /* , */, 5 /* - */, 0 /* . */,
        7 /* / */, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 3 /* : */, 2 /* ; */, 19 /* < */, 21 /* = */, 20 /* > */, 26 /* ? */, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 11 /* [ */, 27, 12 /* ] */, 24 /* ^ */, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 13 /* { */, 23 /* | */, 14 /* } */, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27
};

const char *tokenizer::tokenTypeToString(TokenType type) {
    switch (type) {
        case Period:
            return ".";
        case Comma:
            return ",";
        case Semicolon:
            return ";";
        case Colon:
            return ":";
        case Plus:
            return "+";
        case Minus:
            return "-";
        case Asterisk:
            return "*";
        case ForwardSlash:
            return "/";
        case Percentage:
            return "%";
        case LeftParenthesis:
            return "(";
        case RightParenthesis:
            return ")";
        case LeftBracket:
            return "[";
        case RightBracket:
            return "]";
        case LeftCurly:
            return "{";
        case RightCurly:
            return "}";
        case Less:
            return "<";
        case Greater:
            return ">";
        case LessEqual:
            return "<=";
        case GreaterEqual:
            return ">=";
        case Equal:
            return "==";
        case NotEqual:
            return "!=";
        case Assignment:
            return "=";
        case And:
            return "&";
        case Or:
            return "|";
        case Xor:
            return "^";
        case Not:
            return "!";
        case QuestionMark:
            return "?";
        case BooleanLiteral:
            return "Boolean literal";
        case DoubleLiteral:
            return "Double literal";
        case FloatLiteral:
            return "Float literal";
        case LongLiteral:
            return "Long literal";
        case IntegerLiteral:
            return "Integer literal";
        case ShortLiteral:
            return "Short literal";
        case ByteLiteral:
            return "Byte literal";
        case CharacterLiteral:
            return "Character literal";
        case StringLiteral:
            return "String literal";
        case NothingLiteral:
            return "Nothing literal";
        case Identifier:
            return "Identifier";
        case Unknown:
            return "Unknown";
    }
    return nullptr;
}

void tokenizer::tokenize(Source &source, Array<Token> &tokens, Errors &errors) {
    CharacterStream stream(source);

    while (stream.hasMore()) {
        stream.skipWhiteSpace();
        if (!stream.hasMore()) break;
        stream.startSpan();

        // Numbers
        if (stream.matchDigit(false)) {
            TokenType type = IntegerLiteral;
            if (stream.match("0x", true)) {
                while (stream.matchHex(true));
            } else {
                while (stream.matchDigit(true));
                if (stream.match(".", true)) {
                    type = FloatLiteral;
                    while (stream.matchDigit(true));
                }
            }
            if (stream.match("b", true)) {
                if (type == FloatLiteral) QAK_ERROR(stream.endSpan(), "Byte literal can not have a decimal point.");
                type = ByteLiteral;
            } else if (stream.match("s", true)) {
                if (type == FloatLiteral) QAK_ERROR(stream.endSpan(), "Short literal can not have a decimal point.");
                type = ShortLiteral;
            } else if (stream.match("l", true)) {
                if (type == FloatLiteral) QAK_ERROR(stream.endSpan(), "Long literal can not have a decimal point.");
                type = LongLiteral;
            } else if (stream.match("f", true)) {
                type = FloatLiteral;
            } else if (stream.match("d", true)) {
                type = DoubleLiteral;
            }
            tokens.add({type, stream.endSpan()});
            continue;
        }

        // Character literal
        if (stream.match("'", true)) {
            // Note: escape sequences like \n are parsed in the AST
            stream.match("\\", true);
            stream.consume();
            if (!stream.match("'", true)) QAK_ERROR(stream.endSpan(), "Expected closing ' for character literal.");
            tokens.add({CharacterLiteral, stream.endSpan()});
            continue;
        }

        // String literal
        if (stream.match("\"", true)) {
            bool matchedEndQuote = false;
            while (stream.hasMore()) {
                // Note: escape sequences like \n are parsed in the AST
                if (stream.match("\\", true)) {
                    stream.consume();
                }
                if (stream.match("\"", true)) {
                    matchedEndQuote = true;
                    break;
                }
                if (stream.match("\n", false)) {
                    QAK_ERROR(stream.endSpan(), "String literal is not closed by double quote");
                }
                stream.consume();
            }
            if (!matchedEndQuote) QAK_ERROR(stream.endSpan(), "String literal is not closed by double quote");
            Span stringSpan = stream.endSpan();
            tokens.add({StringLiteral, Span(stringSpan.source, stringSpan.start, stringSpan.startLine, stringSpan.end,
                                            stringSpan.endLine)});
            continue;
        }

        // Identifier, keyword, boolean literal, or null literal
        if (stream.matchIdentifierStart(true)) {
            while (stream.matchIdentifierPart(true));
            Span identifier = stream.endSpan();

            if (identifier.matches(QAK_STR("true")) || identifier.matches(QAK_STR("false"))) {
                tokens.add({BooleanLiteral, identifier});
            } else if (identifier.matches(QAK_STR("nothing"))) {
                tokens.add({NothingLiteral, identifier});
            } else {
                tokens.add({Identifier, identifier});
            }
            continue;
        }

        // Else check for "simple" tokens made up of
        // 1 character literals, like ".", or "[",
        // and 2 character literals.
        uint32_t character = stream.consume();
        TokenType type = Unknown;
        if (character < sizeof(literalToTokenType)) {
            type = (TokenType) literalToTokenType[character];
            if (type == Unknown) QAK_ERROR(stream.endSpan(), "Unknown token");
            if (!stream.hasMore()) {
                tokens.add({(TokenType) type, stream.endSpan()});
                continue;
            }

            uint32_t nextCharacter = stream.peek();
            if (nextCharacter == '=') {
                stream.consume();
                switch (type) {
                    case Less:
                        tokens.add({(TokenType) LessEqual, stream.endSpan()});
                        break;
                    case Greater:
                        tokens.add({(TokenType) GreaterEqual, stream.endSpan()});
                        break;
                    case Not:
                        tokens.add({(TokenType) NotEqual, stream.endSpan()});
                        break;
                    case Assignment:
                        tokens.add({(TokenType) Equal, stream.endSpan()});
                        break;
                    default: QAK_ERROR(stream.endSpan(), "Found unknown two character token");
                }
            } else {
                tokens.add({(TokenType) type, stream.endSpan()});
            }
            continue;
        }

        QAK_ERROR(stream.endSpan(), "Unknown token");
    }
}

void tokenizer::printTokens(Array<Token> &tokens, HeapAllocator &mem) {
    size_t lastLine = 1;
    for (size_t i = 0; i < tokens.size(); i++) {
        Token &token = tokens[i];
        if (token.startLine != lastLine) {
            printf("\n");
            lastLine = token.startLine;
        }
        printf("%s (%d:%d:%d): %s\n", tokenizer::tokenTypeToString(token.type), token.startLine, token.start, token.end,
               token.toCString(mem));
    }
}