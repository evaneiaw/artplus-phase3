#include "pdf_types.h"
#include <sstream>
#include <iomanip>

namespace ArtPlus {
namespace PDF {

double PdfValue::getNumber(const std::string& key, double fallback) const {
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return fallback;
    if (it->second.isNumber()) return it->second.number();
    return fallback;
}

int PdfValue::getInt(const std::string& key, int fallback) const {
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return fallback;
    if (it->second.isInt()) return (int)it->second.intValue();
    if (it->second.isNumber()) return (int)it->second.number();
    return fallback;
}

bool PdfValue::getBool(const std::string& key, bool fallback) const {
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return fallback;
    if (it->second.isBool()) return it->second.boolValue();
    return fallback;
}

QString PdfValue::getName(const std::string& key) const {
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return QString();
    if (it->second.isName()) return it->second.nameValue();
    if (it->second.isString()) return QString::fromLatin1(it->second.stringValue());
    return QString();
}

QByteArray PdfValue::getString(const std::string& key) const {
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return QByteArray();
    if (it->second.isString()) return it->second.stringValue();
    return QByteArray();
}

std::vector<double> PdfValue::getNumberArray(const std::string& key) const {
    std::vector<double> out;
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return out;
    if (!it->second.isArray()) return out;
    for (const auto& v : it->second.array()) {
        if (v.isNumber()) out.push_back(v.number());
        else out.push_back(0.0);
    }
    return out;
}

std::vector<PdfValue> PdfValue::getArray(const std::string& key) const {
    std::vector<PdfValue> out;
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return out;
    if (!it->second.isArray()) return out;
    return it->second.array();
}

std::map<std::string, PdfValue> PdfValue::getDict(const std::string& key) const {
    std::map<std::string, PdfValue> out;
    auto it = m_dict.find(key);
    if (it == m_dict.end()) return out;
    if (!it->second.isDictionary()) return out;
    out = it->second.dict();
    return out;
}

static std::string escapePdfString(const QByteArray& s) {
    std::string out;
    for (unsigned char c : s) {
        if (c == '(' || c == ')' || c == '\\') {
            out += '\\';
            out += (char)c;
        } else if (c >= 32 && c < 127) {
            out += (char)c;
        } else {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\%03o", c);
            out += buf;
        }
    }
    return out;
}

QString PdfValue::toString() const {
    std::ostringstream os;
    switch (m_type) {
        case Type::Null: return "null";
        case Type::Bool: return m_bool ? "true" : "false";
        case Type::Int: {
            os << "int(" << (int64_t)m_num << ")";
            return QString::fromStdString(os.str());
        }
        case Type::Double: {
            os << m_num;
            return QString::fromStdString(os.str());
        }
        case Type::String: {
            std::string s = escapePdfString(m_str);
            return QString("str(%1)").arg(QString::fromStdString(s));
        }
        case Type::Name: return QString("/%1").arg(m_name);
        case Type::Ref: {
            os << m_refObj << " " << m_refGen << " R";
            return QString::fromStdString(os.str());
        }
        case Type::Array: {
            os << "[";
            for (size_t i = 0; i < m_array.size(); ++i) {
                if (i) os << " ";
                os << m_array[i].toString().toStdString();
            }
            os << "]";
            return QString::fromStdString(os.str());
        }
        case Type::Dictionary: {
            os << "<< ";
            for (const auto& [k, v] : m_dict) {
                os << "/" << k << " " << v.toString().toStdString() << " ";
            }
            os << ">>";
            return QString::fromStdString(os.str());
        }
    }
    return "?";
}

} // namespace PDF
} // namespace ArtPlus
