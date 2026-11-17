#pragma once

#include <QPainterPath>
#include <QPolygonF>
#include <QRectF>
#include <QPointF>
#include <QTransform>
#include <vector>
#include <optional>
#include <cmath>

namespace ArtPlus {
namespace Geometry {

constexpr float EPSILON = 1e-6f;
constexpr float PI = 3.14159265358979323846f;

struct Point {
    float x, y;
    Point() : x(0), y(0) {}
    Point(float x_, float y_) : x(x_), y(y_) {}
    Point(const QPointF& p) : x(p.x()), y(p.y()) {}
    operator QPointF() const { return QPointF(x, y); }

    Point operator+(const Point& o) const { return Point(x + o.x, y + o.y); }
    Point operator-(const Point& o) const { return Point(x - o.x, y - o.y); }
    Point operator*(float s) const { return Point(x * s, y * s); }
    Point operator/(float s) const { return Point(x / s, y / s); }
};

inline float dot(const Point& a, const Point& b) { return a.x * b.x + a.y * b.y; }
inline float cross(const Point& a, const Point& b) { return a.x * b.y - a.y * b.x; }
inline float length(const Point& a) { return std::sqrt(dot(a, a)); }
inline Point normalize(const Point& a) { float l = length(a); return l > EPSILON ? a / l : Point(0, 0); }
inline float distance(const Point& a, const Point& b) { return length(a - b); }

struct BezierCurve {
    Point p0, p1, p2, p3;

    Point at(float t) const {
        float u = 1 - t;
        float uu = u * u, uuu = uu * u;
        float tt = t * t, ttt = tt * t;
        return p0 * uuu + p1 * (3 * uu * t) + p2 * (3 * u * tt) + p3 * ttt;
    }

    Point tangent(float t) const {
        float u = 1 - t;
        return (p1 - p0) * (3 * u * u) + (p2 - p1) * (6 * u * t) + (p3 - p2) * (3 * t * t);
    }

    std::pair<BezierCurve, BezierCurve> split(float t) const {
        Point p01 = p0 + (p1 - p0) * t;
        Point p12 = p1 + (p2 - p1) * t;
        Point p23 = p2 + (p3 - p2) * t;
        Point p012 = p01 + (p12 - p01) * t;
        Point p123 = p12 + (p23 - p12) * t;
        Point p0123 = p012 + (p123 - p012) * t;
        return {BezierCurve{p0, p01, p012, p0123}, BezierCurve{p0123, p123, p23, p3}};
    }
};

inline QPainterPath toPainterPath(const std::vector<Point>& points, bool closed = false) {
    QPainterPath path;
    if (points.empty()) return path;
    path.moveTo(points[0]);
    for (size_t i = 1; i < points.size(); ++i) path.lineTo(points[i]);
    if (closed) path.closeSubpath();
    return path;
}

inline std::vector<Point> flattenCurve(const BezierCurve& curve, float tolerance = 0.5f) {
    std::vector<Point> points;
    auto recursive = [&](auto&& self, const BezierCurve& c, float t0, float t1, Point p0, Point p1) -> void {
        float tm = (t0 + t1) * 0.5f;
        Point pm = c.at(tm);
        Point chordMid = (p0 + p1) * 0.5f;
        if (distance(pm, chordMid) < tolerance || (t1 - t0) < 0.01f) {
            points.push_back(p1);
            return;
        }
        auto [left, right] = c.split(0.5f);
        self(self, left, t0, tm, p0, pm);
        self(self, right, tm, t1, pm, p1);
    };
    points.push_back(curve.p0);
    recursive(recursive, curve, 0.0f, 1.0f, curve.p0, curve.p3);
    return points;
}

struct Transform {
    float m11 = 1, m12 = 0, m21 = 0, m22 = 1, dx = 0, dy = 0;

    Transform() = default;
    Transform(const QTransform& t) : m11(t.m11()), m12(t.m12()), m21(t.m21()), m22(t.m22()), dx(t.dx()), dy(t.dy()) {}
    operator QTransform() const { return QTransform(m11, m12, m21, m22, dx, dy); }

    static Transform translate(float tx, float ty) { Transform t; t.dx = tx; t.dy = ty; return t; }
    static Transform scale(float sx, float sy) { Transform t; t.m11 = sx; t.m22 = sy; return t; }
    static Transform rotate(float angle) {
        float c = std::cos(angle), s = std::sin(angle);
        Transform t; t.m11 = c; t.m12 = -s; t.m21 = s; t.m22 = c; return t;
    }
    static Transform shear(float shx, float shy) { Transform t; t.m12 = shx; t.m21 = shy; return t; }

    Point map(const Point& p) const {
        return Point(m11 * p.x + m12 * p.y + dx, m21 * p.x + m22 * p.y + dy);
    }
    Point mapVector(const Point& v) const {
        return Point(m11 * v.x + m12 * v.y, m21 * v.x + m22 * v.y);
    }

    Transform operator*(const Transform& o) const {
        Transform r;
        r.m11 = m11 * o.m11 + m12 * o.m21;
        r.m12 = m11 * o.m12 + m12 * o.m22;
        r.m21 = m21 * o.m11 + m22 * o.m21;
        r.m22 = m21 * o.m12 + m22 * o.m22;
        r.dx = m11 * o.dx + m12 * o.dy + dx;
        r.dy = m21 * o.dx + m22 * o.dy + dy;
        return r;
    }
    Transform inverted() const {
        float det = m11 * m22 - m12 * m21;
        if (std::abs(det) < EPSILON) return Transform();
        Transform r;
        r.m11 = m22 / det; r.m12 = -m12 / det;
        r.m21 = -m21 / det; r.m22 = m11 / det;
        r.dx = -(r.m11 * dx + r.m12 * dy);
        r.dy = -(r.m21 * dx + r.m22 * dy);
        return r;
    }
};

struct BoundingBox {
    float minX = 0, minY = 0, maxX = 0, maxY = 0;
    bool empty = true;

    void add(const Point& p) {
        if (empty) { minX = maxX = p.x; minY = maxY = p.y; empty = false; }
        else { minX = std::min(minX, p.x); maxX = std::max(maxX, p.x); minY = std::min(minY, p.y); maxY = std::max(maxY, p.y); }
    }
    void add(const BoundingBox& o) { if (!o.empty) { add(Point(o.minX, o.minY)); add(Point(o.maxX, o.maxY)); } }
    QRectF toRectF() const { return empty ? QRectF() : QRectF(minX, minY, maxX - minX, maxY - minY); }
    bool contains(const Point& p) const { return !empty && p.x >= minX - EPSILON && p.x <= maxX + EPSILON && p.y >= minY - EPSILON && p.y <= maxY + EPSILON; }
    BoundingBox transformed(const Transform& t) const {
        if (empty) return *this;
        BoundingBox r;
        r.add(t.map(Point(minX, minY)));
        r.add(t.map(Point(maxX, minY)));
        r.add(t.map(Point(minX, maxY)));
        r.add(t.map(Point(maxX, maxY)));
        return r;
    }
};

bool pointInPolygon(const Point& pt, const std::vector<Point>& poly) {
    bool inside = false;
    for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
        const Point& pi = poly[i], &pj = poly[j];
        if (((pi.y > pt.y) != (pj.y > pt.y)) && (pt.x < (pj.x - pi.x) * (pt.y - pi.y) / (pj.y - pi.y) + pi.x))
            inside = !inside;
    }
    return inside;
}

std::vector<Point> offsetPolygon(const std::vector<Point>& poly, float offset) {
    std::vector<Point> result;
    if (poly.size() < 3) return result;
    for (size_t i = 0; i < poly.size(); ++i) {
        Point p0 = poly[(i + poly.size() - 1) % poly.size()];
        Point p1 = poly[i];
        Point p2 = poly[(i + 1) % poly.size()];
        Point v1 = normalize(p1 - p0);
        Point v2 = normalize(p2 - p1);
        Point n1(-v1.y, v1.x);
        Point n2(-v2.y, v2.x);
        Point bisector = normalize(n1 + n2);
        float dotProd = dot(bisector, n1);
        if (std::abs(dotProd) < EPSILON) { result.push_back(p1 + n1 * offset); continue; }
        float dist = offset / dotProd;
        result.push_back(p1 + bisector * dist);
    }
    return result;
}

std::optional<Point> lineIntersection(Point a1, Point a2, Point b1, Point b2) {
    Point da = a2 - a1, db = b2 - b1;
    float crossD = cross(da, db);
    if (std::abs(crossD) < EPSILON) return std::nullopt;
    float t = cross(b1 - a1, db) / crossD;
    return a1 + da * t;
}

bool segmentsIntersect(Point a1, Point a2, Point b1, Point b2) {
    auto ccw = [](Point a, Point b, Point c) { return cross(b - a, c - a) > 0; };
    bool c1 = ccw(a1, b1, b2), c2 = ccw(a2, b1, b2);
    bool c3 = ccw(a1, a2, b1), c4 = ccw(a1, a2, b2);
    return c1 != c2 && c3 != c4;
}

std::vector<Point> polygonBoolean(const std::vector<Point>& subject, const std::vector<Point>& clip, bool intersection = true) {
    std::vector<Point> output = subject;
    for (size_t i = 0; i < clip.size(); ++i) {
        Point A = clip[i], B = clip[(i + 1) % clip.size()];
        std::vector<Point> input = output;
        output.clear();
        if (input.empty()) break;
        Point S = input.back();
        for (const Point& E : input) {
            bool Ein = cross(B - A, E - A) >= -EPSILON;
            bool Sin = cross(B - A, S - A) >= -EPSILON;
            if (Ein) {
                if (!Sin) {
                    if (auto I = lineIntersection(S, E, A, B)) output.push_back(*I);
                }
                output.push_back(E);
            } else if (Sin) {
                if (auto I = lineIntersection(S, E, A, B)) output.push_back(*I);
            }
            S = E;
        }
    }
    return output;
}

QPainterPath pathFromPolygon(const std::vector<Point>& poly) {
    QPainterPath path;
    if (poly.empty()) return path;
    path.moveTo(poly[0]);
    for (size_t i = 1; i < poly.size(); ++i) path.lineTo(poly[i]);
    path.closeSubpath();
    return path;
}

std::vector<Point> polygonFromPath(const QPainterPath& path) {
    std::vector<Point> points;
    for (const auto& el : path.toSubpathPolygons()) {
        for (const auto& pt : el) points.emplace_back(pt);
    }
    return points;
}

struct PathMetrics {
    float length = 0;
    float area = 0;
    Point centroid;
    BoundingBox bounds;

    static PathMetrics compute(const QPainterPath& path) {
        PathMetrics m;
        auto polys = path.toSubpathPolygons();
        for (const auto& poly : polys) {
            for (const auto& pt : poly) m.bounds.add(Point(pt));
            float a = 0;
            Point c(0, 0);
            for (size_t i = 0, n = poly.size(); i < n; ++i) {
                Point p0 = Point(poly[i]), p1 = Point(poly[(i + 1) % n]);
                float cross = p0.x * p1.y - p0.y * p1.x;
                a += cross;
                c.x += (p0.x + p1.x) * cross;
                c.y += (p0.y + p1.y) * cross;
            }
            a *= 0.5f;
            m.area += a;
            if (std::abs(a) > EPSILON) {
                c.x /= (6 * a);
                c.y /= (6 * a);
                m.centroid.x += c.x * a;
                m.centroid.y += c.y * a;
            }
        }
        if (std::abs(m.area) > EPSILON) {
            m.centroid.x /= m.area;
            m.centroid.y /= m.area;
        }
        return m;
    }
};

} // namespace Geometry
} // namespace ArtPlus