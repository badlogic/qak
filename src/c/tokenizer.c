#include "qak.h"

#include "error.h"

static const uint32_t literalToTokenType[] = {
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 25 /* ! */, 27, 27,
        27, 8 /* % */, 22 /* & */, 27, 9 /* ( */, 10 /* ) */, 6 /* * */, 4 /* + */, 1 /* , */, 5 /* - */, 0 /* . */, 7 /* / */, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 3 /* : */, 2 /* ; */, 19 /* < */, 21 /* = */, 20 /* > */, 26 /* ? */, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 11 /* [ */, 27, 12 /* ] */, 24 /* ^ */, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 13 /* { */, 23 /* | */, 14 /* } */, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
        27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27, 27
};

const char *qak_token_type_to_string(qak_token_type type) {
    switch (type) {
        case QakTokenPeriod:
            return ".";
        case QakTokenComma:
            return ",";
        case QakTokenSemicolon:
            return ";";
        case QakTokenColon:
            return ":";
        case QakTokenPlus:
            return "+";
        case QakTokenMinus:
            return "-";
        case QakTokenAsterisk:
            return "*";
        case QakTokenForwardSlash:
            return "/";
        case QakTokenPercentage:
            return "%";
        case QakTokenLeftParenthesis:
            return "(";
        case QakTokenRightParenthesis:
            return ")";
        case QakTokenLeftBracket:
            return "[";
        case QakTokenRightBracket:
            return "]";
        case QakTokenLeftCurly:
            return "{";
        case QakTokenRightCurly:
            return "}";
        case QakTokenLess:
            return "<";
        case QakTokenGreater:
            return ">";
        case QakTokenLessEqual:
            return "<=";
        case QakTokenGreaterEqual:
            return ">=";
        case QakTokenEqual:
            return "==";
        case QakTokenNotEqual:
            return "!=";
        case QakTokenAssignment:
            return "=";
        case QakTokenAnd:
            return "&";
        case QakTokenOr:
            return "|";
        case QakTokenXor:
            return "^";
        case QakTokenNot:
            return "!";
        case QakTokenQuestionMark:
            return "?";
        case QakTokenBooleanLiteral:
            return "Boolean literal";
        case QakTokenDoubleLiteral:
            return "Double literal";
        case QakTokenFloatLiteral:
            return "Float literal";
        case QakTokenLongLiteral:
            return "Long literal";
        case QakTokenIntegerLiteral:
            return "Integer literal";
        case QakTokenShortLiteral:
            return "Short literal";
        case QakTokenByteLiteral:
            return "Byte literal";
        case QakTokenCharacterLiteral:
            return "Character literal";
        case QakTokenStringLiteral:
            return "String literal";
        case QakTokenNothingLiteral:
            return "Nothing literal";
        case QakTokenIdentifier:
            return "Identifier";
        case QakTokenUnknown:
            return "Unknown";
    }
    return NULL;
}

typedef struct qak_character_stream {
    /* The source the stream traverses. */
    const qak_source *source;

    /* The current byte index into the source's data. */
    uint32_t index;

    /* The current line number in the source's data. */
    uint32_t line;

    /* The byte index of the last byte in the source's data + 1. Just a minimal optimization. */
    uint32_t end;

    /* The byte index of the stream the last time CharacterStream::startSpan() was called. */
    uint32_t spanStart;

    /* The line number of the stream the last time CharacterStream::startSpan() was called. */
    uint32_t spanLineStart;
} qak_character_stream;

QAK_INLINE void stream_init(qak_character_stream *stream, qak_source *source) {
    stream->source = source;
    stream->index = 0;
    stream->line = 1;
    stream->end = (uint32_t)source->data.length;
    stream->spanStart = 0;
    stream->spanLineStart = 0;
}

QAK_INLINE uint32_t next_utf8_character(const char *data, uint32_t *index) {
    static const uint32_t utf8Offsets[6] = {
            0x00000000UL, 0x00003080UL, 0x000E2080UL,
            0x03C82080UL, 0xFA082080UL, 0x82082080UL
    };

    uint32_t character = 0;
    int sz = 0;
    do {
        character <<= 6;
        character += data[(*index)++];
        sz++;
    } while (data[*index] && !(((data[*index]) & 0xC0) != 0x80));
    character -= utf8Offsets[sz - 1];

    return character;
}

/* Returns whether the stream has more UTF-8 characters */
QAK_INLINE bool has_more(qak_character_stream *stream) {
    return stream->index < stream->end;
}

/* Returns the current UTF-8 character and advances to the next character */
QAK_INLINE uint32_t consume(qak_character_stream *stream) {
    return next_utf8_character(stream->source->data.data, &stream->index);
}

/* Returns the current UTF-8 character without advancing to the next character */
QAK_INLINE uint32_t peek(qak_character_stream *stream) {
    uint32_t i = stream->index;
    return next_utf8_character(stream->source->data.data, &i);
}

/* Returns true if the current UTF-8 character matches the needle, false otherwise.
 * Advances to the next character if consume is true */
QAK_INLINE bool match(qak_character_stream *stream, const char *needleData, bool consume) {
    uint32_t needleLength = 0;
    const char *sourceData = stream->source->data.data;
    for (uint32_t i = 0, j = stream->index; needleData[i] != 0; i++, needleLength++) {
        if (stream->index >= stream->end) return false;
        uint32_t c = next_utf8_character(sourceData, &j);
        if ((unsigned char) needleData[i] != c) return false;
    }
    if (consume) stream->index += needleLength;
    return true;
}

/* Returns true if the current UTF-8 character is a digit ([0-9]), false otherwise.
 * Advances to the next character if consume is true */
QAK_INLINE bool match_digit(qak_character_stream *stream, bool consume) {
    if (!has_more(stream)) return false;
    char c = stream->source->data.data[stream->index];
    if (c >= '0' && c <= '9') {
        if (consume) stream->index++;
        return true;
    }
    return false;
}

/* Returns true if the current UTF-8 character is a hex-digit ([0-9a-fA-F]), false otherwise.
 * Advances to the next character if consume is true */
QAK_INLINE bool match_hex(qak_character_stream *stream, bool consume) {
    if (!has_more(stream)) return false;
    char c = stream->source->data.data[stream->index];
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        if (consume) stream->index++;
        return true;
    }
    return false;
}

/* Returns true if the current UTF-8 character is valid as the first character
 * of an identifier ([a-zA-Z_] or any unicode character >= 0xc0), e.g. variable
 * name, false otherwise. Advances to the next character if consume is true */
QAK_INLINE bool match_identifier_start(qak_character_stream *stream, bool consume) {
    if (!has_more(stream)) return false;
    uint32_t idx = stream->index;
    const char *sourceData = stream->source->data.data;
    uint32_t c = next_utf8_character(sourceData, &idx);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0xc0) {
        if (consume) stream->index = idx;
        return true;
    }
    return false;
}

/* Returns true if the current UTF-8 character is valid as the first character
 * of an identifier ([a-zA-Z_] or any unicode character >= 0x80), e.g. variable
 * name, false otherwise. Advances to the next character if consume is true */
QAK_INLINE bool match_identifier_part(qak_character_stream *stream, bool consume) {
    if (!has_more(stream)) return false;
    uint32_t idx = stream->index;
    const char *sourceData = stream->source->data.data;
    uint32_t c = next_utf8_character(sourceData, &idx);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9') || c >= 0x80) {
        if (consume) stream->index = idx;
        return true;
    }
    return false;
}

/* Skips all white space characters ([' '\r\n\t]) and single-line comments.
 * Comments start with '#' and end at the end of the current line. */
QAK_INLINE void skip_white_space(qak_character_stream *stream) {
    const char *sourceData = stream->source->data.data;
    while (true) {
        if (stream->index >= stream->end) return;
        char c = sourceData[stream->index];
        switch (c) {
            case '#': {
                while (stream->index < stream->end && c != '\n') {
                    c = sourceData[stream->index];
                    stream->index++;
                }
                stream->line++;
                continue;
            }
            case ' ':
            case '\r':
            case '\t': {
                stream->index++;
                continue;
            }
            case '\n': {
                stream->index++;
                stream->line++;
                continue;
            }
            default:
                return;
        }
    }
}

/* Mark the start of a span at the current position in the stream. See Span::endSpan(). */
QAK_INLINE void start_span(qak_character_stream *stream) {
    stream->spanStart = stream->index;
    stream->spanLineStart = stream->line;
}

/* Return the Span ending at the current position, previously started via
 * startSpan(). Calls to startSpan() and endSpan() must match. They can
 * not be nested.*/
QAK_INLINE qak_span end_span(qak_character_stream *stream) {
    qak_span span;
    span.data.data = stream->source->data.data + stream->spanStart;
    span.data.length = stream->index - stream->spanStart;
    span.startLine = stream->spanLineStart;
    span.endLine = stream->line;

    return span;
}

bool qak_span_matches(qak_span *span, const char *needle, size_t length) {
    if (span->data.length != length) return false;

    const char *sourceData = span->data.data;
    for (uint32_t i = 0; i < length; i++) {
        if (sourceData[i] != needle[i]) return false;
    }
    return true;
}

void qak_tokenize(qak_source *source, qak_array_token *tokens, qak_errors *errors) {
    qak_character_stream stream;
    stream_init(&stream, source);

    while (has_more(&stream)) {
        skip_white_space(&stream);
        if (!has_more(&stream)) break;
        start_span(&stream);

        // Numbers
        if (match_digit(&stream, false)) {
            qak_token_type type = QakTokenIntegerLiteral;
            if (match(&stream, "0x", true)) {
                while (match_hex(&stream, true));
            } else {
                while (match_digit(&stream, true));
                if (match(&stream, ".", true)) {
                    type = QakTokenFloatLiteral;
                    while (match_digit(&stream, true));
                }
            }
            if (match(&stream, "b", true)) {
                if (type == QakTokenFloatLiteral) {
                    qak_errors_add(errors, source, end_span(&stream), "Byte literal can not have a decimal point.");
                    return;
                }
                type = QakTokenByteLiteral;
            } else if (match(&stream, "s", true)) {
                if (type == QakTokenFloatLiteral) {
                    qak_errors_add(errors, source, end_span(&stream), "Short literal can not have a decimal point.");
                    return;
                }
                type = QakTokenShortLiteral;
            } else if (match(&stream, "l", true)) {
                if (type == QakTokenFloatLiteral) {
                    qak_errors_add(errors, source, end_span(&stream), "Long literal can not have a decimal point.");
                    return;
                }
                type = QakTokenLongLiteral;
            } else if (match(&stream, "f", true)) {
                type = QakTokenFloatLiteral;
            } else if (match(&stream, "d", true)) {
                type = QakTokenDoubleLiteral;
            }
            qak_array_token_add(tokens, (qak_token) {type, end_span(&stream)});
            continue;
        }

        // Character literal
        if (match(&stream, "'", true)) {
            // Note: escape sequences like \n are parsed in the AST
            match(&stream, "\\", true);
            consume(&stream);
            if (!match(&stream, "'", true)) {
                qak_errors_add(errors, source, end_span(&stream), "Expected closing ' for character literal.");
                return;
            }

            qak_array_token_add(tokens, (qak_token) {QakTokenCharacterLiteral, end_span(&stream)});
            continue;
        }

        // String literal
        if (match(&stream, "\"", true)) {
            bool matchedEndQuote = false;
            while (has_more(&stream)) {
                // Note: escape sequences like \n are parsed in the AST
                if (match(&stream, "\\", true)) {
                    consume(&stream);
                }
                if (match(&stream, "\"", true)) {
                    matchedEndQuote = true;
                    break;
                }
                if (match(&stream, "\n", false)) {
                    qak_errors_add(errors, source, end_span(&stream), "String literal is not closed by double quote");
                    return;
                }
                consume(&stream);
            }
            if (!matchedEndQuote) {
                qak_errors_add(errors, source, end_span(&stream), "String literal is not closed by double quote");
                return;
            }
            qak_span stringSpan = end_span(&stream);
            qak_array_token_add(tokens, (qak_token) {QakTokenStringLiteral, stringSpan});
            continue;
        }

        // Identifier, keyword, boolean literal, or null literal
        if (match_identifier_start(&stream, true)) {
            while (match_identifier_part(&stream, true));
            qak_span identifier = end_span(&stream);

            if (qak_span_matches(&identifier, QAK_STR("true")) || qak_span_matches(&identifier, QAK_STR("false"))) {
                qak_array_token_add(tokens, (qak_token) {QakTokenBooleanLiteral, identifier});
            } else if (qak_span_matches(&identifier, QAK_STR("nothing"))) {
                qak_array_token_add(tokens, (qak_token) {QakTokenNothingLiteral, identifier});
            } else {
                qak_array_token_add(tokens, (qak_token) {QakTokenIdentifier, identifier});
            }
            continue;
        }

        // Else check for "simple" tokens made up of
        // 1 character literals, like ".", or "[",
        // and 2 character literals.
        uint32_t character = consume(&stream);
        qak_token_type type = QakTokenUnknown;
        if (character < sizeof(literalToTokenType)) {
            type = (qak_token_type) literalToTokenType[character];
            if (type == QakTokenUnknown) {
                qak_errors_add(errors, source, end_span(&stream), "Unknown token");
                return;
            }
            if (!has_more(&stream)) {
                qak_array_token_add(tokens, (qak_token) {(qak_token_type) type, end_span(&stream)});
                continue;
            }

            uint32_t nextCharacter = peek(&stream);
            if (nextCharacter == '=') {
                consume(&stream);
                switch (type) {
                    case QakTokenLess:
                        qak_array_token_add(tokens, (qak_token) {(qak_token_type) QakTokenLessEqual, end_span(&stream)});
                        break;
                    case QakTokenGreater:
                        qak_array_token_add(tokens, (qak_token) {(qak_token_type) QakTokenGreaterEqual, end_span(&stream)});
                        break;
                    case QakTokenNot:
                        qak_array_token_add(tokens, (qak_token) {(qak_token_type) QakTokenNotEqual, end_span(&stream)});
                        break;
                    case QakTokenAssignment:
                        qak_array_token_add(tokens, (qak_token) {(qak_token_type) QakTokenEqual, end_span(&stream)});
                        break;
                    default: {
                        qak_errors_add(errors, source, end_span(&stream), "Found unknown two character token");
                        return;
                    }
                }
            } else {
                qak_array_token_add(tokens, (qak_token) {(qak_token_type) type, end_span(&stream)});
            }
            continue;
        }

        qak_errors_add(errors, source, end_span(&stream), "Unknown token");
    }
}
