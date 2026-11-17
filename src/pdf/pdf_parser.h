#pragma once

#include "pdf_lexer.h"

namespace ArtPlus {
namespace PDF {

// Recursive-descent parser over the lexer with single-token lookahead.
class PdfTokenParser {
public:
    explicit PdfTokenParser(const QByteArray& data) : m_lexer(data) {}

    PdfToken peek() {
        if (!m_hasPeek) {
            m_peek = m_lexer.next();
            m_hasPeek = true;
        }
        return m_peek;
    }

    PdfToken next() {
        if (m_hasPeek) {
            m_hasPeek = false;
            return m_peek;
        }
        return m_lexer.next();
    }

    void skipToDictClose() {
        int depth = 1;
        while (true) {
            PdfToken t = next();
            if (t.type == PdfTokenType::End) return;
            if (t.type == PdfTokenType::DictClose) { depth--; if (depth == 0) return; }
            else if (t.type == PdfTokenType::DictOpen) depth++;
        }
    }

    PdfValue parseValue() {
        PdfToken t = next();
        switch (t.type) {
            case PdfTokenType::Number: {
                // Check for "N G R" reference (indirect object reference)
                double first = t.number;
                PdfToken t2 = peek();
                if (t2.type == PdfTokenType::Number) {
                    PdfLexer saved = m_lexer; // save state before consuming t2
                    next();                   // consume second number
                    PdfToken t3 = peek();
                    if (t3.type == PdfTokenType::Operator && t3.text == "R") {
                        next(); // consume "R"
                        return PdfValue::makeRef((int)first, (int)t2.number);
                    }
                    // Not a ref: it's just consecutive numbers (e.g. in arrays).
                    // Restore lexer so t2 will be read again by the next parseValue().
                    m_lexer = saved;
                    m_hasPeek = false;
                    return PdfValue::makeNumber(first);
                }
                return PdfValue::makeNumber(first);
            }
            case PdfTokenType::String: return PdfValue::makeString(t.str);
            case PdfTokenType::Name:   return PdfValue::makeName(QString::fromStdString(t.text));
            case PdfTokenType::Bool:   return PdfValue::makeBool(t.boolean);
            case PdfTokenType::Null:   return PdfValue::makeNull();
            case PdfTokenType::ArrayOpen: return parseArray();
            case PdfTokenType::DictOpen:  return parseDictionary();
            case PdfTokenType::End:    return PdfValue::makeNull();
            default:
                // Operator used as a value is invalid; be lenient and return null
                return PdfValue::makeNull();
        }
    }

    PdfValue parseArray() {
        PdfValue arr = PdfValue::makeArray();
        while (true) {
            PdfToken t = peek();
            if (t.type == PdfTokenType::ArrayClose) { next(); return arr; }
            if (t.type == PdfTokenType::End) throw PdfError("Unterminated array");
            arr.arrayPush(parseValue());
        }
    }

    PdfValue parseDictionary() {
        PdfValue dict = PdfValue::makeDictionary();
        while (true) {
            PdfToken t = peek();
            if (t.type == PdfTokenType::DictClose) { next(); return dict; }
            if (t.type == PdfTokenType::End) throw PdfError("Unterminated dictionary");
            if (t.type != PdfTokenType::Name) {
                // PDF keys must be names; skip silently for robustness
                next();
                continue;
            }
            std::string key = next().text; // consume name
            PdfValue value = parseValue();
            dict.dictSet(key, value);
        }
    }

private:
    PdfLexer m_lexer;
    PdfToken m_peek;
    bool m_hasPeek = false;
};

} // namespace PDF
} // namespace ArtPlus
