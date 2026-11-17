#pragma once

#include <QByteArray>
#include <QString>
#include <map>
#include <memory>
#include <vector>
#include <optional>
#include <functional>

namespace ArtPlus {
namespace PDF {

// ---------------------------------------------------------------- PdfValue
// Low-level generic model of a PDF object value.
// Dictionaries are kept as std::map for stable ordering.
// ----------------------------------------------------------------

class PdfValue {
public:
    enum class Type {
        Null,
        Bool,
        Int,
        Double,
        String,     // literal or hex string, decoded
        Name,       // /Name
        Array,      // [ a b c ]
        Dictionary, // << /Key val >>
        Ref         // N G R
    };

    PdfValue() : m_type(Type::Null) {}
    PdfValue(Type t) : m_type(t) {}

    static PdfValue makeNull()                 { return PdfValue(Type::Null); }
    static PdfValue makeBool(bool v)           { PdfValue x(Type::Bool); x.m_bool = v; return x; }
    static PdfValue makeInt(int64_t v)         { PdfValue x(Type::Int); x.m_num = (double)v; x.m_isInt = true; return x; }
    static PdfValue makeDouble(double v)       { PdfValue x(Type::Double); x.m_num = v; return x; }
    static PdfValue makeNumber(double v)       { PdfValue x; x.m_type = (v == (double)(int64_t)v) ? Type::Int : Type::Double; x.m_num = v; x.m_isInt = (x.m_type == Type::Int); return x; }
    static PdfValue makeString(const QByteArray& v) { PdfValue x(Type::String); x.m_str = v; return x; }
    static PdfValue makeName(const QString& v) { PdfValue x(Type::Name); x.m_name = v; return x; }
    static PdfValue makeArray()                { return PdfValue(Type::Array); }
    static PdfValue makeDictionary()           { return PdfValue(Type::Dictionary); }
    static PdfValue makeRef(int obj, int gen)  { PdfValue x(Type::Ref); x.m_refObj = obj; x.m_refGen = gen; return x; }

    Type type() const { return m_type; }
    bool isNull() const { return m_type == Type::Null; }
    bool isBool() const { return m_type == Type::Bool; }
    bool isNumber() const { return m_type == Type::Int || m_type == Type::Double; }
    bool isInt() const { return m_type == Type::Int; }
    bool isString() const { return m_type == Type::String; }
    bool isName() const { return m_type == Type::Name; }
    bool isArray() const { return m_type == Type::Array; }
    bool isDictionary() const { return m_type == Type::Dictionary; }
    bool isRef() const { return m_type == Type::Ref; }

    bool boolValue() const { return m_bool; }
    double number() const { return m_num; }
    int64_t intValue() const { return (int64_t)m_num; }
    const QByteArray& stringValue() const { return m_str; }
    QString stringText() const { return QString::fromLatin1(m_str); }
    const QString& nameValue() const { return m_name; }

    // Array access
    std::vector<PdfValue>& array() { return m_array; }
    const std::vector<PdfValue>& array() const { return m_array; }
    size_t arraySize() const { return m_array.size(); }
    PdfValue arrayAt(size_t i) const { return i < m_array.size() ? m_array[i] : PdfValue::makeNull(); }
    void arrayPush(const PdfValue& v) { m_array.push_back(v); }

    // Dictionary access
    std::map<std::string, PdfValue>& dict() { return m_dict; }
    const std::map<std::string, PdfValue>& dict() const { return m_dict; }
    bool dictHas(const std::string& key) const { return m_dict.count(key) != 0; }
    PdfValue dictGet(const std::string& key) const { auto it = m_dict.find(key); return it != m_dict.end() ? it->second : PdfValue::makeNull(); }
    void dictSet(const std::string& key, const PdfValue& v) { m_dict[key] = v; }

    // Convenience helpers
    double getNumber(const std::string& key, double fallback = 0.0) const;
    int getInt(const std::string& key, int fallback = 0) const;
    bool getBool(const std::string& key, bool fallback = false) const;
    QString getName(const std::string& key) const;      // returns name without leading slash
    QByteArray getString(const std::string& key) const; // raw bytes
    std::vector<double> getNumberArray(const std::string& key) const;
    std::vector<PdfValue> getArray(const std::string& key) const;
    std::map<std::string, PdfValue> getDict(const std::string& key) const;

    QString toString() const; // Debug dump

    int refObj() const { return m_refObj; }
    int refGen() const { return m_refGen; }

private:
    Type m_type;
    bool m_bool = false;
    bool m_isInt = false;
    double m_num = 0.0;
    QByteArray m_str;
    QString m_name;
    std::vector<PdfValue> m_array;
    std::map<std::string, PdfValue> m_dict;
    int m_refObj = 0, m_refGen = 0;
};

// ------------------------------------------------------------------ objects
struct PdfObjectId {
    int number = 0;
    int generation = 0;
    bool operator<(const PdfObjectId& o) const {
        return number < o.number || (number == o.number && generation < o.generation);
    }
    bool operator==(const PdfObjectId& o) const {
        return number == o.number && generation == o.generation;
    }
};

struct PdfObject {
    PdfObjectId id;
    PdfValue value;              // dictionary / array / number / ... for non-stream objects
    std::vector<uint8_t> stream; // decoded stream bytes (empty for non-stream)
    bool isStream = false;
    int64_t rawOffset = -1;      // offset of "N G obj" in file (for round-trip export)
};

// --------------------------------------------------------------- exception
struct PdfError : public std::runtime_error {
    PdfError(const std::string& msg) : std::runtime_error(msg) {}
};

} // namespace PDF
} // namespace ArtPlus
