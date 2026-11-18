#include "pdf_lexer.h"
#include <cctype>
#include <stdexcept>

namespace ArtPlus {
namespace PDF {

void PdfLexer::reset(const QByteArray& data) {
    m_data = data;
    m_pos = 0;
}

bool PdfLexer::isWhitespace(unsigned char c) const {
    return c == 0 || c == 9 || c == 10 || c == 12 || c == 13 || c == 32;
}

bool PdfLexer::isDelimiter(unsigned char c) const {
    // PDF delimiter characters
    switch (c) {
        case '(': case ')': case '<': case '>': case '[': case ']':
        case '{': case '}': case '/': case '%':
            return true;
        default:
            return false;
    }
}

void PdfLexer::skipWhitespaceAndComments() {
    while (m_pos < (size_t)m_data.size()) {
        unsigned char c = (unsigned char)m_data[m_pos];
        if (isWhitespace(c)) { m_pos++; continue; }
        if (c == '%') { // comment to end of line
            while (m_pos < (size_t)m_data.size() && m_data[m_pos] != '\n' && m_data[m_pos] != '\r') m_pos++;
            continue;
        }
        break;
    }
}

PdfToken PdfLexer::lexNumber() {
    size_t start = m_pos;
    bool negative = false;
    if (m_data[m_pos] == '+' || m_data[m_pos] == '-') {
        negative = (m_data[m_pos] == '-');
        m_pos++;
    }
    bool any = false;
    while (m_pos < (size_t)m_data.size() && std::isdigit((unsigned char)m_data[m_pos])) {
        m_pos++;
        any = true;
    }
    if (m_pos < (size_t)m_data.size() && m_data[m_pos] == '.') {
        m_pos++;
        while (m_pos < (size_t)m_data.size() && std::isdigit((unsigned char)m_data[m_pos])) {
            m_pos++;
            any = true;
        }
    }
    if (!any) {
        throw PdfError("Invalid number at offset " + std::to_string(start));
    }
    std::string text(m_data.constData() + start, m_pos - start);
    // text already includes the leading sign (+/-)
    double value = std::stod(text);
    (void)negative;
    PdfToken tok;
    tok.type = PdfTokenType::Number;
    tok.number = value;
    tok.text = text;
    return tok;
}

PdfToken PdfLexer::lexString() {
    // Literal string: ( ... ) with balanced parens and backslash escapes
    size_t start = m_pos;
    m_pos++; // consume '('
    int depth = 1;
    QByteArray out;
    while (m_pos < (size_t)m_data.size()) {
        unsigned char c = (unsigned char)m_data[m_pos];
        if (c == '\\') {
            m_pos++;
            if (m_pos >= (size_t)m_data.size()) break;
            c = (unsigned char)m_data[m_pos];
            switch (c) {
                case 'n': out.append('\n'); m_pos++; break;
                case 'r': out.append('\r'); m_pos++; break;
                case 't': out.append('\t'); m_pos++; break;
                case 'b': out.append('\b'); m_pos++; break;
                case 'f': out.append('\f'); m_pos++; break;
                case '(': out.append('('); m_pos++; break;
                case ')': out.append(')'); m_pos++; break;
                case '\\': out.append('\\'); m_pos++; break;
                default: {
                    if (std::isdigit(c)) {
                        // octal escape up to 3 digits
                        std::string oct;
                        while (m_pos < (size_t)m_data.size() && oct.size() < 3 && std::isdigit((unsigned char)m_data[m_pos])) {
                            oct += (char)m_data[m_pos];
                            m_pos++;
                        }
                        int code = std::stoi(oct, nullptr, 8);
                        out.append((char)code);
                    } else {
                        out.append((char)c);
                        m_pos++;
                    }
                    break;
                }
            }
            continue;
        }
        if (c == '(') { depth++; out.append('('); m_pos++; continue; }
        if (c == ')') {
            depth--;
            m_pos++;
            if (depth == 0) {
                PdfToken tok;
                tok.type = PdfTokenType::String;
                tok.str = out;
                tok.text = std::string(out.constData(), out.size());
                return tok;
            }
            out.append(')');
            continue;
        }
        out.append((char)c);
        m_pos++;
    }
    throw PdfError("Unterminated string starting at offset " + std::to_string(start));
}

PdfToken PdfLexer::lexHexString() {
    // < ... > with hex digits, whitespace ignored
    size_t start = m_pos;
    m_pos++; // consume '<'
    std::string hex;
    while (m_pos < (size_t)m_data.size()) {
        unsigned char c = (unsigned char)m_data[m_pos];
        if (c == '>') { m_pos++; break; }
        if (std::isxdigit(c)) hex += (char)c;
        m_pos++;
    }
    if (hex.size() % 2) hex += '0';
    QByteArray out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        int byte = std::stoi(hex.substr(i, 2), nullptr, 16);
        out.append((char)byte);
    }
    PdfToken tok;
    tok.type = PdfTokenType::String;
    tok.str = out;
    tok.text = std::string(out.constData(), out.size());
    return tok;
}

PdfToken PdfLexer::lexName() {
    size_t start = m_pos;
    m_pos++; // consume '/'
    std::string raw;
    while (m_pos < (size_t)m_data.size()) {
        unsigned char c = (unsigned char)m_data[m_pos];
        if (isWhitespace(c) || isDelimiter(c)) break;
        if (c == '#') {
            // /#XX hex escape
            if (m_pos + 2 < (size_t)m_data.size()) {
                std::string hex = m_data.mid((int)m_pos + 1, 2).toStdString();
                int v = std::stoi(hex, nullptr, 16);
                raw += (char)v;
                m_pos += 3;
                continue;
            }
        }
        raw += (char)c;
        m_pos++;
    }
    PdfToken tok;
    tok.type = PdfTokenType::Name;
    tok.text = raw;
    return tok;
}

PdfToken PdfLexer::lexWord() {
    size_t start = m_pos;
    while (m_pos < (size_t)m_data.size()) {
        unsigned char c = (unsigned char)m_data[m_pos];
        if (isWhitespace(c) || isDelimiter(c)) break;
        m_pos++;
    }
    std::string word = m_data.mid((int)start, (int)(m_pos - start)).toStdString();
    PdfToken tok;
    if (word == "true") {
        tok.type = PdfTokenType::Bool; tok.boolean = true; tok.text = word; return tok;
    }
    if (word == "false") {
        tok.type = PdfTokenType::Bool; tok.boolean = false; tok.text = word; return tok;
    }
    if (word == "null") {
        tok.type = PdfTokenType::Null; tok.text = word; return tok;
    }
    tok.type = PdfTokenType::Operator;
    tok.text = word;
    return tok;
}

PdfToken PdfLexer::next() {
    skipWhitespaceAndComments();
    if (m_pos >= (size_t)m_data.size()) {
        PdfToken tok;
        tok.type = PdfTokenType::End;
        return tok;
    }

    unsigned char c = (unsigned char)m_data[m_pos];

    // numbers
    if (std::isdigit(c) || c == '+' || c == '-' || c == '.') {
        return lexNumber();
    }
    // strings
    if (c == '(') return lexString();
    if (c == '<') {
        if (m_pos + 1 < (size_t)m_data.size() && m_data[m_pos + 1] == '<') {
            m_pos += 2;
            PdfToken tok;
            tok.type = PdfTokenType::DictOpen;
            tok.text = "<<";
            return tok;
        }
        return lexHexString();
    }
    if (c == '>') {
        if (m_pos + 1 < (size_t)m_data.size() && m_data[m_pos + 1] == '>') {
            m_pos += 2;
            PdfToken tok;
            tok.type = PdfTokenType::DictClose;
            tok.text = ">>";
            return tok;
        }
        throw PdfError("Unexpected '>' at offset " + std::to_string(m_pos));
    }
    if (c == '[') {
        m_pos++;
        PdfToken tok;
        tok.type = PdfTokenType::ArrayOpen;
        tok.text = "[";
        return tok;
    }
    if (c == ']') {
        m_pos++;
        PdfToken tok;
        tok.type = PdfTokenType::ArrayClose;
        tok.text = "]";
        return tok;
    }
    if (c == '/') {
        return lexName();
    }

    return lexWord();
}

} // namespace PDF
} // namespace ArtPlus
