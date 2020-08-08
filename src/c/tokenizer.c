#include "qak.h"

#include "error.h"

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

static QAK_FORCE_INLINE void qak_character_stream_init(qak_character_stream *stream, qak_source *source) {
    stream->source = source;
    stream->index = 0;
    stream->line = 0;
    stream->end = source->data.length;
    stream->spanStart = 0;
    stream->spanLineStart = 0;
}

static QAK_FORCE_INLINE uint32_t qak_character_stream_next_utf8_character(const uint8_t *data, uint32_t *index) {
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
static QAK_FORCE_INLINE bool qak_character_stream_has_more(qak_character_stream *stream) {
    return stream->index < stream->end;
}

/* Returns whether the stream has numChars more UTF-8 characters */
static QAK_FORCE_INLINE bool qak_character_stream_has_more_characters(qak_character_stream *stream, size_t numChars) {
    return stream->index + numChars - 1 < stream->end;
}

/* Returns the current UTF-8 character and advances to the next character */
static QAK_FORCE_INLINE uint32_t qak_character_stream_consume(qak_character_stream *stream) {
    return qak_character_stream_next_utf8_character(stream->source->data.data, &stream->index);
}

/* Returns the current UTF-8 character without advancing to the next character */
static QAK_FORCE_INLINE uint32_t qak_character_stream_peek(qak_character_stream *stream) {
    uint32_t i = stream->index;
    return qak_character_stream_next_utf8_character(stream->source->data.data, &i);
}

/* Returns true if the current UTF-8 character matches the needle, false otherwise.
 * Advances to the next character if consume is true */
static QAK_FORCE_INLINE bool qak_character_stream_match(qak_character_stream *stream, const char *needleData, bool consume) {
    uint32_t needleLength = 0;
    const uint8_t *sourceData = stream->source->data.data;
    for (uint32_t i = 0, j = stream->index; needleData[i] != 0; i++, needleLength++) {
        if (stream->index >= stream->end) return false;
        uint32_t c = qak_character_stream_next_utf8_character(sourceData, &j);
        if ((unsigned char) needleData[i] != c) return false;
    }
    if (consume) stream->index += needleLength;
    return true;
}

/* Returns true if the current UTF-8 character is a digit ([0-9]), false otherwise.
 * Advances to the next character if consume is true */
static QAK_FORCE_INLINE bool qak_character_stream_match_digit(qak_character_stream *stream, bool consume) {
    if (!qak_character_stream_has_more(stream)) return false;
    uint8_t c = stream->source->data.data[stream->index];
    if (c >= '0' && c <= '9') {
        if (consume) stream->index++;
        return true;
    }
    return false;
}

/* Returns true if the current UTF-8 character is a hex-digit ([0-9a-fA-F]), false otherwise.
 * Advances to the next character if consume is true */
static QAK_FORCE_INLINE bool qak_character_stream_match_hex(qak_character_stream *stream, bool consume) {
    if (!qak_character_stream_has_more(stream)) return false;
    uint8_t c = stream->source->data.data[stream->index];
    if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
        if (consume) stream->index++;
        return true;
    }
    return false;
}

/* Returns true if the current UTF-8 character is valid as the first character
 * of an identifier ([a-zA-Z_] or any unicode character >= 0xc0), e.g. variable
 * name, false otherwise. Advances to the next character if consume is true */
static QAK_FORCE_INLINE bool qak_character_stream_match_identifier_start(qak_character_stream *stream, bool consume) {
    if (!qak_character_stream_has_more(stream)) return false;
    uint32_t idx = stream->index;
    const uint8_t *sourceData = stream->source->data.data;
    uint32_t c = qak_character_stream_next_utf8_character(sourceData, &idx);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0xc0) {
        if (consume) stream->index = idx;
        return true;
    }
    return false;
}

/* Returns true if the current UTF-8 character is valid as the first character
 * of an identifier ([a-zA-Z_] or any unicode character >= 0x80), e.g. variable
 * name, false otherwise. Advances to the next character if consume is true */
static QAK_FORCE_INLINE bool qak_character_stream_match_identifier_part(qak_character_stream *stream, bool consume) {
    if (!qak_character_stream_has_more(stream)) return false;
    uint32_t idx = stream->index;
    const uint8_t *sourceData = stream->source->data.data;
    uint32_t c = qak_character_stream_next_utf8_character(sourceData, &idx);
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9') || c >= 0x80) {
        if (consume) stream->index = idx;
        return true;
    }
    return false;
}

/* Skips all white space characters ([' '\r\n\t]) and single-line comments.
 * Comments start with '#' and end at the end of the current line. */
static QAK_FORCE_INLINE void qak_character_stream_skip_whiteSpace(qak_character_stream *stream) {
    const uint8_t *sourceData = stream->source->data.data;
    while (true) {
        if (stream->index >= stream->end) return;
        uint8_t c = sourceData[stream->index];
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
static QAK_FORCE_INLINE void qak_character_stream_start_span(qak_character_stream *stream) {
    stream->spanStart = stream->index;
    stream->spanLineStart = stream->line;
}

/* Return the Span ending at the current position, previously started via
 * startSpan(). Calls to startSpan() and endSpan() must match. They can
 * not be nested.*/
static QAK_FORCE_INLINE qak_span qak_character_stream_end_span(qak_character_stream *stream) {
    qak_span span;
    span.data = stream->source->data;
    span.start = stream->spanStart;
    span.end = stream->index;
    span.startLine = stream->spanLineStart;
    span.endLine = stream->line;

    return span;
}

static const uint32_t literalToTokenType[] = {
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        25, // !
        28,
        26, // #
        28,
        8, // %
        22, // &
        28,
        9, // (
        10, // )
        6, // *
        4, // +
        1, // ,
        5, // -
        0, // .
        7, // /
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        3, // :
        2, // ;
        19, // <
        21, // =
        20, // >
        27, // ?
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        11, // [
        28,
        12, // ]
        24, // ^
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        13, // {
        23, // |
        14, // }
};

bool qak_span_matches(qak_span *span, const char* needle, size_t length) {
    if (span->end - span->start != length) return false;

    const uint8_t *sourceData = span->data.data;
    for (uint32_t i = 0; i < length; i++) {
        if (sourceData[i] != needle[i]) return false;
    }
    return true;
}

void qak_tokenize(qak_source *source, qak_array_token *tokens, qak_array_error *errors) {
    qak_character_stream stream;
    qak_character_stream_init(&stream, source);

    while (qak_character_stream_has_more(&stream)) {
        qak_character_stream_skip_whiteSpace(&stream);
        if (!qak_character_stream_has_more(&stream)) break;
        qak_character_stream_start_span(&stream);

        // Numbers
        if (qak_character_stream_match_digit(&stream, false)) {
            qak_token_type type = QakTokenIntegerLiteral;
            if (qak_character_stream_match(&stream, "0x", true)) {
                while (qak_character_stream_match_hex(&stream, true));
            } else {
                while (qak_character_stream_match_digit(&stream, true));
                if (qak_character_stream_match(&stream, ".", true)) {
                    type = QakTokenFloatLiteral;
                    while (qak_character_stream_match_digit(&stream, true));
                }
            }
            if (qak_character_stream_match(&stream, "b", true)) {
                if (type == QakTokenFloatLiteral) QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Byte literal can not have a decimal point.");
                type = QakTokenByteLiteral;
            } else if (qak_character_stream_match(&stream, "s", true)) {
                if (type == QakTokenFloatLiteral) QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Short literal can not have a decimal point.");
                type = QakTokenShortLiteral;
            } else if (qak_character_stream_match(&stream, "l", true)) {
                if (type == QakTokenFloatLiteral) QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Long literal can not have a decimal point.");
                type = QakTokenLongLiteral;
            } else if (qak_character_stream_match(&stream, "f", true)) {
                type = QakTokenFloatLiteral;
            } else if (qak_character_stream_match(&stream, "d", true)) {
                type = QakTokenDoubleLiteral;
            }
            qak_array_token_add(tokens, (qak_token) {type, qak_character_stream_end_span(&stream)});
            continue;
        }

        // Character literal
        if (qak_character_stream_match(&stream, "'", true)) {
            // Note: escape sequences like \n are parsed in the AST
            qak_character_stream_match(&stream, "\\", true);
            qak_character_stream_consume(&stream);
            if (!qak_character_stream_match(&stream, "'", true)) QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Expected closing ' for character literal.");
            qak_array_token_add(tokens, (qak_token) {QakTokenCharacterLiteral, qak_character_stream_end_span(&stream)});
            continue;
        }

        // String literal
        if (qak_character_stream_match(&stream, "\"", true)) {
            bool matchedEndQuote = false;
            while (qak_character_stream_has_more(&stream)) {
                // Note: escape sequences like \n are parsed in the AST
                if (qak_character_stream_match(&stream, "\\", true)) {
                    qak_character_stream_consume(&stream);
                }
                if (qak_character_stream_match(&stream, "\"", true)) {
                    matchedEndQuote = true;
                    break;
                }
                if (qak_character_stream_match(&stream, "\n", false)) {
                    QAK_ERROR(errors, qak_character_stream_end_span(&stream), "String literal is not closed by double quote");
                }
                qak_character_stream_consume(&stream);
            }
            if (!matchedEndQuote) QAK_ERROR(errors, qak_character_stream_end_span(&stream), "String literal is not closed by double quote");
            qak_span stringSpan = qak_character_stream_end_span(&stream);
            qak_array_token_add(tokens, (qak_token) {QakTokenStringLiteral, stringSpan});
            continue;
        }

        // Identifier, keyword, boolean literal, or null literal
        if (qak_character_stream_match_identifier_start(&stream, true)) {
            while (qak_character_stream_match_identifier_start(&stream, true));
            qak_span identifier = qak_character_stream_end_span(&stream);

            if (qak_span_matches(&identifier, QAK_STR("true")) || qak_span_matches(&identifier, QAK_STR("false"))) {
                qak_array_token_add(tokens, (qak_token){QakTokenBooleanLiteral, identifier});
            } else if (qak_span_matches(&identifier, QAK_STR("nothing"))) {
                qak_array_token_add(tokens, (qak_token){QakTokenNothingLiteral, identifier});
            } else {
                qak_array_token_add(tokens, (qak_token){QakTokenIdentifier, identifier});
            }
            continue;
        }

        // Else check for "simple" tokens made up of
        // 1 character literals, like ".", or "[",
        // and 2 character literals.
        uint32_t character = qak_character_stream_consume(&stream);
        qak_token_type type = QakTokenUnknown;
        if (character < sizeof(literalToTokenType)) {
            type = (qak_token_type) literalToTokenType[character];
            if (type == QakTokenUnknown) QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Unknown token");
            if (!qak_character_stream_has_more(&stream)) {
                qak_array_token_add(tokens, (qak_token){(qak_token_type) type, qak_character_stream_end_span(&stream)});
                continue;
            }

            uint32_t nextCharacter = qak_character_stream_peek(&stream);
            if (nextCharacter == '=') {
                qak_character_stream_consume(&stream);
                switch (type) {
                    case QakTokenLess:
                        qak_array_token_add(tokens, (qak_token){(qak_token_type) QakTokenLessEqual, qak_character_stream_end_span(&stream)});
                        break;
                    case QakTokenGreater:
                        qak_array_token_add(tokens, (qak_token){(qak_token_type) QakTokenGreaterEqual, qak_character_stream_end_span(&stream)});
                        break;
                    case QakTokenNot:
                        qak_array_token_add(tokens, (qak_token){(qak_token_type) QakTokenNotEqual, qak_character_stream_end_span(&stream)});
                        break;
                    case QakTokenAssignment:
                        qak_array_token_add(tokens, (qak_token){(qak_token_type) QakTokenEqual, qak_character_stream_end_span(&stream)});
                        break;
                    default: QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Found unknown two character token");
                }
            } else {
                qak_array_token_add(tokens, (qak_token) {(qak_token_type) type, qak_character_stream_end_span(&stream)});
            }
            continue;
        }

        QAK_ERROR(errors, qak_character_stream_end_span(&stream), "Unknown token");
    }
}