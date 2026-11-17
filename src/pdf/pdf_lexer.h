#pragma once

#include "pdf_types.h"
#include <sstream>

namespace ArtPlus {
namespace PDF {

// Lightweight PDF tokenizer. Produces one of:
//   NUMBER, STRING, NAME, BOOL, NULL,
//   DICT_OPEN (<<), DICT_CLOSE (>>),
//   ARRAY_OPEN ([), ARRAY_CLOSE (]),
//   REF marker is signalled via NUMBER NUMBER R pattern handled by the parser,
//   OPERATOR (any other word, e.g. "m", "f", "BT"),
//   END (end of input)
enum class PdfTokenType {
    Number,
    String,
    Name,
    Bool,
    Null,
    DictOpen,   // <<
    DictClose,  // >>
    ArrayOpen,  // [
    ArrayClose, // ]
    Operator,   // operators and keywords; parser decides meaning
    End
};

struct PdfToken {
    PdfTokenType type = PdfTokenType::End;
    std::string text;    // raw text for Operator/Name (name without '/')/String
    QByteArray str;      // decoded bytes for String
    double number = 0.0;
    bool boolean = false;
};

class PdfLexer {
public:
    PdfLexer() = default;
    explicit PdfLexer(const QByteArray& data) { reset(data); }

    void reset(const QByteArray& data);
    PdfToken next();
    void skipWhitespaceAndComments();
    bool atEnd() const { return m_pos >= m_data.size(); }
    size_t position() const { return m_pos; }
    void seek(size_t pos) { m_pos = pos; }

private:
    QByteArray m_data;
    size_t m_pos = 0;

    PdfToken lexNumber();
    PdfToken lexString();
    PdfToken lexHexString();
    PdfToken lexName();
    PdfToken lexWord();
    bool isDelimiter(unsigned char c) const;
    bool isWhitespace(unsigned char c) const;
};

} // namespace PDF
} // namespace ArtPlus
