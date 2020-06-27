#include "tokenizer.h"

using namespace qak;

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
        case Hash:
            return "#";
        case QuestionMark:
            return "?";
        case DoubleQuote:
            return "\"";
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
        case NullLiteral:
            return "Null literal";
        case Identifier:
            return "Identifier";
        case LastSimpleTokenType:
            return "Last simple token type marker";
    }
    return nullptr;
}

void tokenizer::tokenize(Source &source, Array<Token> &tokens, Errors &errors) {
    CharacterStream stream(source);

    while (stream.hasMore()) {
        stream.skipWhiteSpace();
        if (!stream.hasMore()) break;

        // Numbers
        if (stream.matchDigit(false)) {
            TokenType type = IntegerLiteral;
            stream.startSpan();
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
        if (stream.match("'", false)) {
            stream.startSpan();
            stream.consume();
            // Note: escape sequences like \n are parsed in the AST
            stream.match("\\", true);
            stream.consume();
            if (!stream.match("'", true)) QAK_ERROR(stream.endSpan(), "Expected closing ' for character literal.");
            tokens.add({CharacterLiteral, stream.endSpan()});
            continue;
        }

        // String literal
        if (stream.match("\"", true)) {
            stream.startSpan();
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
            tokens.add({StringLiteral, Span(stringSpan.source, stringSpan.start - 1, stringSpan.startLine, stringSpan.end, stringSpan.endLine)});
            continue;
        }

        // Identifier, keyword, boolean literal, or null literal
        if (stream.matchIdentifierStart(false)) {
            stream.startSpan();
            stream.consume();
            while (stream.matchIdentifierPart(true));
            Span identifier = stream.endSpan();

            if (identifier.match(QAK_STR("true")) || identifier.match(QAK_STR("false"))) {
                tokens.add({BooleanLiteral, identifier});
            } else if (identifier.match(QAK_STR("null"))) {
                tokens.add({NullLiteral, identifier});
            } else {
                tokens.add({Identifier, identifier});
            }
            continue;
        }

        // Simple tokens, start with first type in enum,
        // iterate until LastSimpleTokenType.
        uint32_t type = Period;
        stream.startSpan();
        bool foundSimple = false;
        while (type != LastSimpleTokenType) {
            const char *literal = tokenTypeToString((TokenType) type);
            if (literal != nullptr) {
                if (stream.match(literal, true)) {
                    tokens.add({(TokenType) type, stream.endSpan()});
                    foundSimple = true;
                    break;
                }
            }
            type++;
        }

        if (!foundSimple) QAK_ERROR(stream.endSpan(), "Unknown token");
    }
}