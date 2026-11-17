#include "pdf_document.h"
#include "pdf_parser.h"
#include <QtZlib/zlib.h>
#include <QFile>
#include <QRegularExpression>
#include <cstring>
#include <cctype>
#include <cstdio>

namespace ArtPlus {
namespace PDF {

// =============================================================== helpers
static QByteArray readRawStream(const QByteArray& data, int64_t streamStart, int64_t length) {
    if (streamStart < 0) streamStart = 0;
    int64_t end = streamStart + length;
    if (end > data.size()) end = data.size();
    return data.mid((int)streamStart, (int)(end - streamStart));
}

// ================================================================== open
bool PdfDocument::open(const QString& filePath) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    m_bytes = f.readAll();
    f.close();
    m_path = filePath;
    if (m_bytes.size() < 16 || !m_bytes.startsWith("%PDF-")) return false;

    // locate startxref
    int64_t pos = m_bytes.lastIndexOf("startxref");
    if (pos >= 0) {
        int64_t p = pos + 9;
        while (p < m_bytes.size() && !std::isdigit((unsigned char)m_bytes[p])) p++;
        QByteArray num;
        while (p < m_bytes.size() && std::isdigit((unsigned char)m_bytes[p])) {
            num += m_bytes[p];
            p++;
        }
        m_startXref = num.toLongLong();
    }

    try {
        parseAll();
        if (m_objects.empty()) return false;
        // Collect page refs from the catalog via the trailer dictionary
        int64_t t = m_bytes.lastIndexOf("trailer");
        if (t < 0) t = m_bytes.indexOf("/Type /Catalog"); // fallback for xref streams
        if (t >= 0) {
            // Find the dictionary that contains /Root
            // Approach: find lastOccurrence of "/Root"
            int64_t r = m_bytes.lastIndexOf("/Root");
            if (r < 0) return false;
            // parse from the dict start: walk backwards to find '<<'
            int64_t d = m_bytes.lastIndexOf("<<", (int)r);
            if (d < 0) return false;
            PdfTokenParser parser(m_bytes.mid((int)d));
            PdfValue trailer = parser.parseDictionary();
            PdfValue root = trailer.dictGet("Root");
            PdfValue rootObj = resolveValue(root);
            if (!rootObj.isDictionary()) return false;
            PdfValue pages = resolveValue(rootObj.dictGet("Pages"));
            collectPages(pages);
            // info dict
            PdfValue info = trailer.dictGet("Info");
            if (info.isRef() || info.isDictionary()) {
                PdfValue infoObj = resolveValue(info);
                auto getStr = [&](const std::string& key) -> QString {
                    auto it = infoObj.dict().find(key);
                    if (it == infoObj.dict().end()) return QString();
                    if (it->second.isString()) {
                        QByteArray raw = it->second.stringValue();
                        if (raw.size() >= 2 && (unsigned char)raw[0] == 0xFE && (unsigned char)raw[1] == 0xFF) {
                            return QString::fromUtf16(reinterpret_cast<const char16_t*>(raw.constData() + 2), (raw.size() - 2) / 2);
                        }
                        return QString::fromLatin1(raw);
                    }
                    return QString();
                };
                m_info.title = getStr("Title");
                m_info.author = getStr("Author");
                m_info.subject = getStr("Subject");
                m_info.keywords = getStr("Keywords");
                m_info.creator = getStr("Creator");
                m_info.producer = getStr("Producer");
                m_info.creationDate = getStr("CreationDate");
                m_info.modificationDate = getStr("ModDate");
            }
            m_info.pageCount = (int)m_pageRefs.size();
        } else {
            // Try to build info from a /Catalog dictionary found anywhere
            int64_t cat = m_bytes.indexOf("/Type /Catalog");
            if (cat >= 0) {
                int64_t d = m_bytes.lastIndexOf("<<", (int)cat);
                if (d >= 0) {
                    PdfTokenParser parser(m_bytes.mid((int)d));
                    PdfValue catalog = parser.parseDictionary();
                    PdfValue pages = resolveValue(catalog.dictGet("Pages"));
                    collectPages(pages);
                    m_info.pageCount = (int)m_pageRefs.size();
                }
            }
        }
        m_open = true;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// ===================================================== parse all objects
void PdfDocument::parseAll() {
    m_objects.clear();
    const QByteArray& data = m_bytes;
    int64_t pos = 0;
    const int64_t n = data.size();

    while (pos < n) {
        // skip whitespace and comments
        while (pos < n) {
            unsigned char c = (unsigned char)data[pos];
            if (c == 0 || c == 9 || c == 10 || c == 12 || c == 13 || c == 32) { pos++; continue; }
            if (c == '%') { while (pos < n && data[pos] != '\n' && data[pos] != '\r') pos++; continue; }
            break;
        }
        if (pos >= n) break;

        int64_t objStart = pos;
        int64_t p = pos;

        // parse "N G obj"
        QByteArray tok1;
        while (p < n && (std::isdigit((unsigned char)data[p]) || data[p] == '-' || data[p] == '+')) { tok1 += data[p]; p++; }
        if (tok1.isEmpty()) { pos = p + 1; continue; }
        while (p < n && (data[p] == ' ' || data[p] == '\t' || data[p] == '\r' || data[p] == '\n')) p++;
        QByteArray tok2;
        while (p < n && std::isdigit((unsigned char)data[p])) { tok2 += data[p]; p++; }
        while (p < n && (data[p] == ' ' || data[p] == '\t' || data[p] == '\r' || data[p] == '\n')) p++;

        if (p + 3 <= n && data.mid((int)p, 3) == "obj") {
            int objNum = tok1.toInt();
            int gen = tok2.isEmpty() ? 0 : tok2.toInt();
            p += 3;

            PdfObject obj;
            obj.id = {objNum, gen};
            obj.rawOffset = objStart;

            int64_t endobj = data.indexOf("endobj", (int)p);
            if (endobj < 0) { pos = p; continue; }

            QByteArray body = data.mid((int)p, (int)(endobj - p));
            int64_t streamIdx = body.indexOf("stream");

            if (streamIdx >= 0) {
                // Stream object: parse dict from the bytes before "stream"
                PdfTokenParser parser(body.left((int)streamIdx));
                PdfToken t = parser.peek();
                if (t.type == PdfTokenType::DictOpen) {
                    obj.value = parser.parseDictionary();
                }
                obj.isStream = true;

                int64_t dataStart = streamIdx + 6; // after "stream"
                while (dataStart < body.size() && (body[dataStart] == '\r' || body[dataStart] == '\n')) dataStart++;

                int64_t length = -1;
                if (obj.value.dictHas("Length")) {
                    PdfValue lenV = obj.value.dictGet("Length");
                    if (lenV.isNumber()) length = (int64_t)lenV.number();
                    else if (lenV.isRef()) {
                        auto it = m_objects.find({lenV.refObj(), lenV.refGen()});
                        if (it != m_objects.end() && it->second.value.isNumber()) length = (int64_t)it->second.value.number();
                    }
                }

                int64_t rawStreamStart = p + dataStart;
                int64_t rawStreamEnd;
                if (length >= 0 && rawStreamStart + length <= endobj) {
                    rawStreamEnd = rawStreamStart + length;
                } else {
                    int64_t es = body.indexOf("endstream", (int)streamIdx);
                    if (es > 0) {
                        int64_t endData = es;
                        while (endData > streamIdx && (body[endData - 1] == '\n' || body[endData - 1] == '\r')) endData--;
                        rawStreamEnd = p + endData;
                    } else {
                        rawStreamEnd = endobj;
                    }
                }

                QByteArray rawStream = m_bytes.mid((int)rawStreamStart, (int)(rawStreamEnd - rawStreamStart));
                std::vector<uint8_t> decoded = decodeStream(obj.value.dict(), rawStream);
                obj.stream.assign(decoded.begin(), decoded.end());
            } else {
                // Non-stream object: parse the value
                PdfTokenParser parser(body);
                obj.value = parser.parseValue();
                obj.isStream = false;
            }
            m_objects[obj.id] = std::move(obj);
            pos = endobj + 6;
            continue;
        }
        pos = p;
    }
}

// ============================================================ xref parse
void PdfDocument::parseXref(int64_t offset) {
    // Informational only; real parsing happens in parseAll().
    (void)offset;
}

// ======================================================= page collection
void PdfDocument::collectPages(const PdfValue& node, int depth) {
    if (depth > 32) return;
    PdfValue type = node.dictGet("Type");
    QString typeName = type.isName() ? type.nameValue() : QString();
    if (typeName == "Page") {
        return; // refs are pushed by the caller
    }
    PdfValue kids = node.dictGet("Kids");
    if (!kids.isArray()) return;
    for (const auto& kidRef : kids.array()) {
        if (!kidRef.isRef()) continue;
        PdfValue kid = resolveValue(kidRef);
        if (!kid.isDictionary()) continue;
        QString kidType;
        PdfValue kt = kid.dictGet("Type");
        if (kt.isName()) kidType = kt.nameValue();
        if (kidType == "Page") {
            m_pageRefs.push_back({kidRef.refObj(), kidRef.refGen()});
        } else if (kidType == "Pages") {
            collectPages(kid, depth + 1);
        }
    }
}

// ======================================================= resolve values
PdfValue PdfDocument::resolveValue(const PdfValue& v) const {
    if (!v.isRef()) return v;
    PdfObjectId id{v.refObj(), v.refGen()};
    auto cacheIt = m_resolvedCache.find(id);
    if (cacheIt != m_resolvedCache.end()) return cacheIt->second;
    auto it = m_objects.find(id);
    if (it == m_objects.end()) return PdfValue::makeNull();
    PdfValue resolved = it->second.value;
    m_resolvedCache[id] = resolved;
    return resolved;
}

PdfValue PdfDocument::resolve(const PdfValue& value) const {
    return resolveValue(value);
}

const PdfObject* PdfDocument::findObject(int obj, int gen) const {
    auto it = m_objects.find({obj, gen});
    return it != m_objects.end() ? &it->second : nullptr;
}

PdfValue PdfDocument::pageDictionary(int pageIndex) const {
    if (pageIndex < 0 || pageIndex >= (int)m_pageRefs.size()) return PdfValue::makeNull();
    PdfObjectId id = m_pageRefs[pageIndex];
    auto it = m_objects.find(id);
    if (it == m_objects.end()) return PdfValue::makeNull();
    return it->second.value;
}

PageGeometry PdfDocument::pageGeometry(int pageIndex) const {
    PageGeometry g;
    PdfValue page = pageDictionary(pageIndex);
    auto readBox = [&](const std::string& key, double fallback[4]) {
        auto arr = page.getNumberArray(key);
        if (arr.size() == 4) {
            for (int i = 0; i < 4; ++i) fallback[i] = arr[i];
        }
    };
    readBox("MediaBox", g.mediaBox);
    readBox("CropBox", g.cropBox);
    readBox("BleedBox", g.bleedBox);
    readBox("TrimBox", g.trimBox);
    readBox("ArtBox", g.artBox);
    if (!page.dictHas("CropBox")) for (int i = 0; i < 4; ++i) g.cropBox[i] = g.mediaBox[i];
    if (!page.dictHas("BleedBox")) for (int i = 0; i < 4; ++i) g.bleedBox[i] = g.mediaBox[i];
    if (!page.dictHas("TrimBox")) for (int i = 0; i < 4; ++i) g.trimBox[i] = g.mediaBox[i];
    if (!page.dictHas("ArtBox")) for (int i = 0; i < 4; ++i) g.artBox[i] = g.mediaBox[i];
    if (page.dictHas("Rotate")) {
        PdfValue r = page.dictGet("Rotate");
        if (r.isNumber()) g.rotation = r.number();
    }
    return g;
}

PdfValue PdfDocument::pageResources(int pageIndex) const {
    PdfValue page = pageDictionary(pageIndex);
    PdfValue res = page.dictGet("Resources");
    if (res.isRef()) res = resolveValue(res);
    return res;
}

QByteArray PdfDocument::pageContents(int pageIndex) const {
    PdfValue page = pageDictionary(pageIndex);
    PdfValue contents = page.dictGet("Contents");
    QByteArray out;
    std::vector<PdfValue> refs;
    if (contents.isRef()) refs.push_back(contents);
    else if (contents.isArray()) refs = contents.array();
    for (const auto& ref : refs) {
        PdfObjectId id;
        if (ref.isRef()) id = {ref.refObj(), ref.refGen()};
        else continue;
        auto it = m_objects.find(id);
        if (it != m_objects.end() && it->second.isStream) {
            out += QByteArray((const char*)it->second.stream.data(), (int)it->second.stream.size());
            out += '\n';
        }
    }
    return out;
}

// ========================================================== stream decode
QByteArray PdfDocument::eolAt(const QByteArray& data, int64_t offset) {
    if (offset >= data.size()) return QByteArray();
    if (data[offset] == '\r' && offset + 1 < data.size() && data[offset + 1] == '\n') return "\r\n";
    if (data[offset] == '\n') return "\n";
    if (data[offset] == '\r') return "\r";
    return QByteArray();
}

std::vector<uint8_t> PdfDocument::ascii85Decode(const QByteArray& in) {
    std::vector<uint8_t> out;
    std::string clean;
    for (char c : in.toStdString()) {
        if (std::isspace((unsigned char)c)) continue;
        clean += c;
    }
    size_t i = 0;
    auto decodeGroup = [&](const char* group, int count) {
        if (count == 0) return;
        uint32_t value = 0;
        for (int j = 0; j < 5; ++j) {
            if (j < count + 1) value = value * 85 + (uint32_t)(unsigned char)group[j] - 33;
            else value = value * 85 + 84;
        }
        uint8_t bytes[4] = {
            (uint8_t)((value >> 24) & 0xFF),
            (uint8_t)((value >> 16) & 0xFF),
            (uint8_t)((value >> 8) & 0xFF),
            (uint8_t)(value & 0xFF)
        };
        for (int b = 0; b < count; ++b) out.push_back(bytes[b]);
    };
    while (i < clean.size()) {
        if (clean[i] == '~' && i + 1 < clean.size() && clean[i + 1] == '>') break;
        if (clean[i] == 'z') { out.insert(out.end(), {0, 0, 0, 0}); i++; continue; }
        char group[5] = {0, 0, 0, 0, 0};
        int count = 0;
        while (i < clean.size() && count < 5 && clean[i] != '~') {
            group[count++] = clean[i++];
        }
        if (count == 0) break;
        decodeGroup(group, count - 1);
    }
    return out;
}

std::vector<uint8_t> PdfDocument::asciiHexDecode(const QByteArray& in) {
    std::vector<uint8_t> out;
    std::string hex;
    for (char c : in.toStdString()) {
        if (std::isxdigit((unsigned char)c)) hex += c;
    }
    if (hex.size() % 2) hex += '0';
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back((uint8_t)std::stoi(hex.substr(i, 2), nullptr, 16));
    }
    return out;
}

std::vector<uint8_t> PdfDocument::flateDecode(const QByteArray& in) {
    std::vector<uint8_t> out;
    z_stream strm = {};
    strm.next_in = (Bytef*)in.constData();
    strm.avail_in = in.size();
    strm.total_in = 0;
    if (inflateInit(&strm) != Z_OK) {
        return out;
    }
    size_t capacity = in.size() * 8 + 1024;
    if (capacity < 65536) capacity = 65536;
    out.resize(capacity);
    int ret = Z_OK;
    size_t total = 0;
    while (ret == Z_OK) {
        if (total >= out.size()) out.resize(out.size() * 2);
        strm.next_out = out.data() + total;
        strm.avail_out = (uInt)(out.size() - total);
        ret = inflate(&strm, Z_NO_FLUSH);
        total = out.size() - strm.avail_out;
    }
    inflateEnd(&strm);
    if (ret == Z_STREAM_END || total > 0) {
        out.resize(total);
        return out;
    }
    return {};
}

std::vector<uint8_t> PdfDocument::decodeStream(const std::map<std::string, PdfValue>& dict, const QByteArray& raw) const {
    std::vector<std::string> filters;
    auto fit = dict.find("Filter");
    if (fit != dict.end()) {
        if (fit->second.isName()) filters = {fit->second.nameValue().toStdString()};
        else if (fit->second.isArray()) {
            for (const auto& v : fit->second.array()) {
                if (v.isName()) filters.push_back(v.nameValue().toStdString());
            }
        }
    }
    if (filters.empty()) {
        return std::vector<uint8_t>(raw.begin(), raw.end());
    }
    QByteArray data = raw;
    for (const auto& f : filters) {
        std::vector<uint8_t> decoded;
        if (f == "ASCII85Decode") decoded = ascii85Decode(data);
        else if (f == "ASCIIHexDecode") decoded = asciiHexDecode(data);
        else if (f == "FlateDecode") decoded = flateDecode(data);
        else {
            // Unsupported filter: return raw bytes rather than silently corrupting
            return std::vector<uint8_t>(data.begin(), data.end());
        }
        data = QByteArray((const char*)decoded.data(), (int)decoded.size());
    }
    return std::vector<uint8_t>(data.begin(), data.end());
}

} // namespace PDF
} // namespace ArtPlus
