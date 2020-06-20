#ifndef QAK_TOKENIZER_H
#define QAK_TOKENIZER_H

#include "array.h"
#include "error.h"

namespace qak {
    struct CharacterStream {
        Source source;
        u4 index;
        u4 end;
        u4 spanStart;

        CharacterStream(Source source);

        bool hasMore();

        u4 peek();

        u4 consume();

        bool match(const char *needle, bool consume);

        bool matchDigit(bool consume);

        bool matchHex(bool consume);

        bool matchIdentifierStart(bool consume);

        bool matchIdentifierPart(bool consume);

        void skipWhiteSpace();

        void startSpan();

        Span endSpan();

        bool isSpanEmpty();
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

        bool match(const char* needle) {
            size_t len = strlen(needle);
            if (span.getLength() != len) return false;
            const u1* sourceData = span.source.buffer.data + span.start;
            for (u4 i = 0; i < len; i++) {
                if (sourceData[i] != needle[i]) return false;
            }
            return true;
        }

        const char *toCString(HeapAllocator &mem) {
            return span.toCString(mem);
        }
    };

    namespace tokenizer {
        void tokenize(Source, Array<Token> &tokens, Errors &errors);

        const char *tokenTypeToString(TokenType type);
    };
}

#endif //QAK_TOKENIZER_H
