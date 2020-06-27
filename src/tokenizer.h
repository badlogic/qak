#ifndef QAK_TOKENIZER_H
#define QAK_TOKENIZER_H

#include "array.h"
#include "error.h"

#define QAK_STR(str) str, sizeof(str) - 1

namespace qak {
    struct CharacterStream {
        Source &source;
        uint32_t index;
        uint32_t line;
        uint32_t end;
        uint32_t spanStart;
        uint32_t spanLineStart;


        // Taken from https://www.cprogramming.com/tutorial/utf8.c
        static QAK_FORCE_INLINE uint32_t nextUtf8Character(const uint8_t *s, uint32_t *i) {
            static const uint32_t utf8Offsets[6] = {
                    0x00000000UL, 0x00003080UL, 0x000E2080UL,
                    0x03C82080UL, 0xFA082080UL, 0x82082080UL
            };

            uint32_t ch = 0;
            int sz = 0;

            do {
                ch <<= 6;
                ch += s[(*i)++];
                sz++;
            } while (s[*i] && !(((s[*i]) & 0xC0) != 0x80));
            ch -= utf8Offsets[sz - 1];

            return ch;
        }

        CharacterStream(Source &source) : source(source), index(0), line(1), end(source.size), spanStart(0), spanLineStart(1) {
        }

        QAK_FORCE_INLINE bool hasMore() {
            return index < end;
        }

        QAK_FORCE_INLINE uint32_t consume() {
            return nextUtf8Character(source.data, &index);
        }

        QAK_FORCE_INLINE bool match(const char *needleData, bool consume) {
            uint32_t needleLength = 0;
            const uint8_t *sourceData = source.data;
            for (uint32_t i = 0, j = index; needleData[i] != 0; i++, needleLength++) {
                if (index >= end) return false;
                uint32_t c = nextUtf8Character(sourceData, &j);
                if ((unsigned char) needleData[i] != c) return false;
            }
            if (consume) index += needleLength;
            return true;
        }

        QAK_FORCE_INLINE bool matchDigit(bool consume) {
            if (!hasMore()) return false;
            uint8_t c = source.data[index];
            if (c >= '0' && c <= '9') {
                if (consume) index++;
                return true;
            }
            return false;
        }

        QAK_FORCE_INLINE bool matchHex(bool consume) {
            if (!hasMore()) return false;
            uint8_t c = source.data[index];
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                if (consume) index++;
                return true;
            }
            return false;
        }

        QAK_FORCE_INLINE bool matchIdentifierStart(bool consume) {
            if (!hasMore()) return false;
            uint32_t idx = index;
            uint32_t c = nextUtf8Character(source.data, &idx);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0xc0) {
                if (consume) index = idx;
                return true;
            }
            return false;
        }

        QAK_FORCE_INLINE bool matchIdentifierPart(bool consume) {
            if (!hasMore()) return false;
            uint32_t idx = index;
            uint32_t c = nextUtf8Character(source.data, &idx);
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9') || c >= 0x80) {
                if (consume) index = idx;
                return true;
            }
            return false;
        }

        QAK_FORCE_INLINE void skipWhiteSpace() {
            const uint8_t *sourceData = source.data;
            while (true) {
                if (index >= end) return;
                uint8_t c = sourceData[index];
                switch (c) {
                    case '#': {
                        while (index < end && c != '\n') {
                            c = sourceData[index];
                            index++;
                        }
                        continue;
                    }
                    case ' ':
                    case '\r':
                    case '\t': {
                        index++;
                        continue;
                    }
                    case '\n': {
                        index++;
                        line++;
                        continue;
                    }
                    default:
                        return;
                }
            }
        }

        QAK_FORCE_INLINE void startSpan() {
            spanStart = index;
            spanLineStart = line;
        }

        QAK_FORCE_INLINE Span endSpan() {
            return {source, spanStart, spanLineStart, index, line};
        }

        bool isSpanEmpty() {
            return spanStart == index;
        }

    };

    enum TokenType {
        // Simple tokens, sorted by literal length. Longer literals first.
        // The list of simple tokens is terminated via LastSimpleTokenType.
        Period,
        Comma,
        Semicolon,
        Colon,
        Plus,
        Minus,
        Asterisk,
        ForwardSlash,
        Percentage,
        LeftParenthesis,
        RightParenthesis,
        LeftBracket,
        RightBracket,
        LeftCurly,
        RightCurly,
        LessEqual,
        GreaterEqual,
        NotEqual,
        Equal,
        Less,
        Greater,
        Assignment,
        And,
        Or,
        Xor,
        Not,
        Hash,
        QuestionMark,
        DoubleQuote,
        LastSimpleTokenType,

        // These don't have a literal representation
        BooleanLiteral,
        DoubleLiteral,
        FloatLiteral,
        LongLiteral,
        IntegerLiteral,
        ShortLiteral,
        ByteLiteral,
        CharacterLiteral,
        StringLiteral,
        NullLiteral,
        Identifier
    };

    struct Token {
        TokenType type;
        Span span;

        QAK_FORCE_INLINE bool match(const char *needle, uint32_t len) {
            if (span.length() != len) return false;
            const uint8_t *sourceData = span.source.data + span.start;
            for (uint32_t i = 0; i < len; i++) {
                if (sourceData[i] != needle[i]) return false;
            }
            return true;
        }

        const char *toCString(HeapAllocator &mem) {
            return span.toCString(mem);
        }
    };

    namespace tokenizer {
        void tokenize(Source &source, Array<Token> &tokens, Errors &errors);

        const char *tokenTypeToString(TokenType type);
    };

    struct TokenStream {
        Source &source;
        Array<Token> &tokens;
        Errors &errors;
        int index;
        int end;

        TokenStream(Source &source, Array<Token> &tokens, Errors &errors) : source(source), tokens(tokens), errors(errors), index(0), end(tokens.size()) {}

        /** Returns whether there are more tokens in the stream. **/
        QAK_FORCE_INLINE bool hasMore() {
            return index < end;
        }

        /** Consumes the next token and returns it. **/
        QAK_FORCE_INLINE Token *consume() {
            if (!hasMore()) return nullptr;
            return &tokens[index++];
        }

        QAK_FORCE_INLINE Token *peek() {
            if (!hasMore()) return nullptr;
            return &tokens[index];
        }

        /** Checks if the next token has the give type and optionally consumes, or throws an error if the next token did not match the
         * type. */
        QAK_FORCE_INLINE Token *expect(TokenType type) {
            bool result = match(type, true);
            if (!result) {
                Token *token = (uint64_t) index < tokens.size() ? &tokens[index] : nullptr;
                Span *span = token != nullptr ? &token->span : nullptr;
                if (span == nullptr)
                    errors.add({source, (uint32_t) source.size - 1, (uint32_t) source.lines().size() + 1, (uint32_t) source.size - 1,
                                (uint32_t) source.lines().size() + 1}, "Expected '%s', but reached the end of the source.", tokenizer::tokenTypeToString(type));
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
        QAK_FORCE_INLINE Token *expect(const char *text, uint32_t len) {
            bool result = match(text, len, true);
            if (!result) {
                Token *token = (uint64_t) index < tokens.size() ? &tokens[index] : nullptr;
                Span *span = token != nullptr ? &token->span : nullptr;
                if (span == nullptr) {
                    errors.add({source, (uint32_t) source.size - 1, (uint32_t) source.lines().size() + 1, (uint32_t) source.size - 1,
                                (uint32_t) source.lines().size() + 1}, "Expected '%s', but reached the end of the source.", text);
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
        QAK_FORCE_INLINE bool match(TokenType type, bool consume) {
            if (index >= end) return false;
            if (tokens[index].type == type) {
                if (consume) index++;
                return true;
            }
            return false;
        }

        /** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
        QAK_FORCE_INLINE bool unsafeMatch(TokenType type, bool consume) {
            if (tokens[index].type == type) {
                if (consume) index++;
                return true;
            }
            return false;
        }

        /** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
        QAK_FORCE_INLINE bool match(const char *text, uint32_t len, bool consume) {
            if (index >= end) return false;
            if (tokens[index].match(text, len)) {
                if (consume) index++;
                return true;
            }
            return false;
        }
    };
}

#endif //QAK_TOKENIZER_H
