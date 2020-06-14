#include "tokenizer.h"

using namespace qak;

CharacterStream::CharacterStream(Source source) : source(source), index(0), end(source.buffer.size), spanStart(0) {
}

bool CharacterStream::hasMore() {
    return index < end;
}

u1 CharacterStream::consume() {
    return source.buffer.data[index++];
}

bool CharacterStream::match(const char *needleData, bool consume) {
    u4 needleLength = 0;
    const u1 *sourceData = source.buffer.data;
    for (u4 i = 0, j = index; needleData[i] != 0; i++, j++, needleLength++) {
        if (index >= end) return false;
        if (needleData[i] != sourceData[j]) return false;
    }
    if (consume) index += needleLength;
    return true;
}

bool CharacterStream::matchDigit(bool consume) {
    if (!hasMore()) return false;
    u1 c = source.buffer.data[index];
    if (c >= '0' && c <= '9') {
        if (consume) index++;
        return true;
    }
    return false;
}

bool CharacterStream::matchHex(bool consume) {
    if (!hasMore()) return false;
    u1 c = source.buffer.data[index];
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        if (consume) index++;
        return true;
    }
    return false;
}

bool CharacterStream::matchIdentifierStart(bool consume) {
    if (!hasMore()) return false;
    u1 c = source.buffer.data[index];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        if (consume) index++;
        return true;
    }
    return false;
}

bool CharacterStream::matchIdentifierPart(bool consume) {
    if (!hasMore()) return false;
    u1 c = source.buffer.data[index];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9')) {
        if (consume) index++;
        return true;
    }
    return false;
}

void CharacterStream::skipWhiteSpace() {
    const u1 *sourceData = source.buffer.data;
    while (true) {
        if (index >= end) return;
        u1 c = sourceData[index];
        if (c == '#') {
            while (index < end && c != '\n') {
                c = sourceData[index];
                index++;
            }
            continue;
        }
        if (c == ' ' || c == '\n' || c == '\r' || c == '\t') {
            index++;
            continue;
        } else {
            break;
        }
    }
}

void CharacterStream::startSpan() {
    spanStart = index;
}

Span CharacterStream::endSpan() {
    return {source, spanStart, index};
}

bool CharacterStream::isSpanEmpty() {
    return spanStart == index;
}

const char *qak::tokenTypeToString(TokenType type) {
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
    }
    return nullptr;
}

void qak::tokenize(Source source, Array<Token> &tokens, Array<Error> &errors) {
    CharacterStream stream(source);

    outer:
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
                if (type == FloatLiteral) ERROR("Byte literal can not have a decimal point.", stream.endSpan());
                type = ByteLiteral;
            } else if (stream.match("s", true)) {
                if (type == FloatLiteral) ERROR("Short literal can not have a decimal point.", stream.endSpan());
                type = ShortLiteral;
            } else if (stream.match("l", true)) {
                if (type == FloatLiteral) ERROR("Long literal can not have a decimal point.", stream.endSpan());
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
            if (!stream.match("'", true)) ERROR("Expected closing ' for character literal.", stream.endSpan());
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
                stream.consume();
            }
            if (!matchedEndQuote) ERROR("String literal is not closed by double quote", stream.endSpan());
            Span stringSpan = stream.endSpan();
            tokens.add({StringLiteral, Span(stringSpan.source, stringSpan.start - 1, stringSpan.end)});
            continue;
        }

        // Identifier, keyword, boolean literal, or null literal
        if (stream.matchIdentifierStart(false)) {
            stream.startSpan();
            stream.consume();
            while (stream.matchIdentifierPart(true));
            Span identifier = stream.endSpan();

            if (identifier.match("true") || identifier.match("false")) {
                tokens.add({BooleanLiteral, identifier});
            } else if (identifier.match("null")) {
                tokens.add({NullLiteral, identifier});
            } else {
                tokens.add({Identifier, identifier});
            }
            continue;
        }

        // Simple tokens, start with first type in enum,
        // iterate until LastSimpleTokenType.
        u4 type = Period;
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

        if (!foundSimple) ERROR("Unknown token", stream.endSpan());
    }
}