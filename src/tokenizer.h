#ifndef QAK_TOKENIZER_H
#define QAK_TOKENIZER_H

#include "array.h"
#include "error.h"

#define QAK_STR(str) str, sizeof(str) - 1

namespace qak {

    /**
     * A CharacterStream is used to traverse a UTF-8 Source. The stream
     * keeps track of the current position within the Source.
     *
     * The stream provides various method to check and/or consume the next
     * character in the source.
     */
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

        /** Returns whether the stream has more UTF-8 characters **/
        QAK_FORCE_INLINE bool hasMore() {
            return index < end;
        }

        /** Returns the current UTF-8 character and advances to the next character **/
        QAK_FORCE_INLINE uint32_t consume() {
            return nextUtf8Character(source.data, &index);
        }

        /** Returns true if the current UTF-8 character matches the needle, false otherwise.
         * Advances to the next character if consume is true **/
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

        /** Returns true if the current UTF-8 character is a digit ([0-9]), false otherwise.
         * Advances to the next character if consume is true **/
        QAK_FORCE_INLINE bool matchDigit(bool consume) {
            if (!hasMore()) return false;
            uint8_t c = source.data[index];
            if (c >= '0' && c <= '9') {
                if (consume) index++;
                return true;
            }
            return false;
        }

        /** Returns true if the current UTF-8 character is a hex-digit ([0-9a-z]), false otherwise.
         * Advances to the next character if consume is true **/
        QAK_FORCE_INLINE bool matchHex(bool consume) {
            if (!hasMore()) return false;
            uint8_t c = source.data[index];
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
                if (consume) index++;
                return true;
            }
            return false;
        }

        /** Returns true if the current UTF-8 character is valid as the first character
         * of an identifier ([a-zA-Z_] or any unicode character >= 0xc0), e.g. variable
         * name, false otherwise. Advances to the next character if consume is true **/
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

        /** Returns true if the current UTF-8 character is valid as the first character
         * of an identifier ([a-zA-Z_] or any unicode character >= 0x80), e.g. variable
         * name, false otherwise. Advances to the next character if consume is true **/
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

        /** Skips all white space characters ([' '\r\n\t]) and single-line comments.
         * Comments start with '#' and end at the end of the current line. */
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
                        line++;
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

        /* Keep track of a Span starting at the current position in the stream **/
        QAK_FORCE_INLINE void startSpan() {
            spanStart = index;
            spanLineStart = line;
        }

        /** Return the Span ending at the current position, previously started via
         * startSpan(). Calls to startSpan() and endSpan() must match. They can
         * not be nested.*/
        QAK_FORCE_INLINE Span endSpan() {
            return {source, spanStart, spanLineStart, index, line};
        }
    };

    /** Enum describing all token types understood by Qak.
     *
     * "Simple" token types
     * are things like operators, brackets, etc. with a single literal representation.
     * If two "simple" tokens start with the same character, the longer one must come
     * first, e.g. "<=" must come before "<". The list of simple tokens is delimited by
     * the enum value LastSimpleTokenType.
     *
     * Token types with more than one literal representation, e.g. identifiers of variables
     * or floating point numbers, come after simple tokens. Their order in the enum is not
     * important.
     */
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

    /** A token with a TokenType and position in the Source expressed by a Span. **/
    struct Token : public Span {
        TokenType type;

        Token(TokenType type, Span span) : Span(span), type(type) {}
    };

    namespace tokenizer {
        /** Tokenizes the Source and returns the found tokens in the tokens array.
         * Errors that occured during parsing are stored in the Errors instance. */
        void tokenize(Source &source, Array<Token> &tokens, Errors &errors);

        /** Returns a string representation for the token type, e.g. TokenType::Identifier
         * returns "Identifier". */
        const char *tokenTypeToString(TokenType type);

        /** Prints the tokens to stdout, grouping them by line. */
        void printTokens(Array<Token> &tokens, HeapAllocator &mem);
    }

    /**
     * A TokenStream is used to traverse a list of Tokens The stream
     * keeps track of the current token.
     *
     * The stream provides various method to match and/or consume the next
     * token.
     */
    class TokenStream {
    private:
        Source &_source;
        Array<Token> &_tokens;
        Errors &_errors;
        size_t _index;

    public:
        TokenStream(Source &source, Array<Token> &tokens, Errors &errors) : _source(source), _tokens(tokens), _errors(errors), _index(0) {}

        /** Returns whether there are more tokens in the stream. **/
        QAK_FORCE_INLINE bool hasMore() {
            return _index < _tokens.size();
        }

        /** Consumes the next token and returns it. **/
        QAK_FORCE_INLINE Token *consume() {
            if (!hasMore()) return nullptr;
            return &_tokens[_index++];
        }

        QAK_FORCE_INLINE Token *peek() {
            if (!hasMore()) return nullptr;
            return &_tokens[_index];
        }

        /** Checks if the next token has the give type and optionally consumes, or throws an error if the next token did not match the
         * type. */
        QAK_FORCE_INLINE Token *expect(TokenType type) {
            bool result = match(type, true);
            if (!result) {
                Token *token = (uint64_t) _index < _tokens.size() ? &_tokens[_index] : nullptr;
                if (token == nullptr)
                    _errors.add({_source, (uint32_t) _source.size - 1, (uint32_t) _source.lines().size() - 1, (uint32_t) _source.size - 1,
                                 (uint32_t) _source.lines().size() - 1}, "Expected '%s', but reached the end of the source.",
                                tokenizer::tokenTypeToString(type));
                else {
                    HeapAllocator mem;
                    _errors.add(*token, "Expected '%s', but got '%s'", tokenizer::tokenTypeToString(type), token->toCString(mem));
                }
                return nullptr;
            } else {
                return &_tokens[_index - 1];
            }
        }

        /** Checks if the next token matches the given text and optionally consumes, or throws an error if the next token did not match
         * the text. */
        QAK_FORCE_INLINE Token *expect(const char *text, uint32_t len) {
            bool result = match(text, len, true);
            if (!result) {
                Token *token = (uint64_t) _index < _tokens.size() ? &_tokens[_index] : nullptr;
                if (token == nullptr) {
                    _errors.add({_source, (uint32_t) _source.size - 1, (uint32_t) _source.lines().size() - 1, (uint32_t) _source.size - 1,
                                 (uint32_t) _source.lines().size() - 1}, "Expected '%s', but reached the end of the source.", text);
                } else {
                    HeapAllocator mem;
                    _errors.add(*token, "Expected '%s', but got '%s'", text, token->toCString(mem));
                }
                return nullptr;
            } else {
                return &_tokens[_index - 1];
            }
        }

        /** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
        QAK_FORCE_INLINE bool match(TokenType type, bool consume) {
            if (_index >= _tokens.size()) return false;
            if (_tokens[_index].type == type) {
                if (consume) _index++;
                return true;
            }
            return false;
        }

        /** Matches and optionally consumes the next token in case of a match. Returns whether the token matched. */
        QAK_FORCE_INLINE bool match(const char *text, uint32_t len, bool consume) {
            if (_index >= _tokens.size()) return false;
            if (_tokens[_index].match(text, len)) {
                if (consume) _index++;
                return true;
            }
            return false;
        }
    };
}

#endif //QAK_TOKENIZER_H
