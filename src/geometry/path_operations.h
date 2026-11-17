#pragma once

#include "geometry_engine.h"
#include <QPainterPath>
#include <vector>
#include <optional>

namespace ArtPlus {
namespace Geometry {

enum class BooleanOp { Union, Intersection, Difference, Xor };

class PathOperations {
public:
    static QPainterPath boolean(const QPainterPath& a, const QPainterPath& b, BooleanOp op) {
        QPainterPath result;
        switch (op) {
            case BooleanOp::Union: result = a.united(b); break;
            case BooleanOp::Intersection: result = a.intersected(b); break;
            case BooleanOp::Difference: result = a.subtracted(b); break;
            case BooleanOp::Xor: result = a.xored(b); break;
        }
        return result;
    }

    static QPainterPath offset(const QPainterPath& path, float distance, Qt::PenJoinStyle joinStyle = Qt::MiterJoin, float miterLimit = 10.0f) {
        QPainterPathStroker stroker;
        stroker.setWidth(std::abs(distance) * 2.0f);
        stroker.setJoinStyle(joinStyle);
        stroker.setMiterLimit(miterLimit);
        stroker.setCapStyle(Qt::FlatCap);
        QPainterPath stroked = stroker.createStroke(path);
        if (distance > 0) return path.united(stroked);
        return path.subtracted(stroked);
    }

    static QPainterPath simplify(const QPainterPath& path, float tolerance = 1.0f) {
        QPainterPath result;
        for (const auto& poly : path.toSubpathPolygons()) {
            if (poly.size() < 3) continue;
            std::vector<Point> simplified = douglasPeucker(polygonFromPath(QPainterPath(poly)), tolerance);
            if (simplified.size() >= 3) result.addPath(pathFromPolygon(simplified));
        }
        return result;
    }

    static QPainterPath smooth(const QPainterPath& path, float cornerRadius) {
        QPainterPath result;
        for (const auto& poly : path.toSubpathPolygons()) {
            if (poly.size() < 3) continue;
            result.addPath(roundCorners(poly, cornerRadius));
        }
        return result;
    }

    static QPainterPath joinPaths(const std::vector<QPainterPath>& paths, float tolerance = 0.5f) {
        if (paths.empty()) return QPainterPath();
        QPainterPath result = paths[0];
        for (size_t i = 1; i < paths.size(); ++i) {
            result = connectEndpoints(result, paths[i], tolerance);
        }
        return result;
    }

    static std::vector<QPainterPath> splitPath(const QPainterPath& path, const QPointF& point, float tolerance = 5.0f) {
        std::vector<QPainterPath> result;
        for (const auto& poly : path.toSubpathPolygons()) {
            auto [left, right] = splitPolygonAtPoint(polygonFromPath(QPainterPath(poly)), Point(point), tolerance);
            if (!left.empty()) result.push_back(pathFromPolygon(left));
            if (!right.empty()) result.push_back(pathFromPolygon(right));
        }
        return result;
    }

    static QPainterPath retracePath(const QPainterPath& path, float tolerance = 1.0f) {
        return simplify(path, tolerance);
    }

    static QPainterPath adjustCorners(const QPainterPath& path, float radius, bool round = true) {
        return round ? smooth(path, radius) : path;
    }

    static std::pair<QPainterPath, QPainterPath> cutPath(const QPainterPath& path, const QLineF& line) {
        QPainterPath left, right;
        QPolygonF poly = path.toFillPolygon();
        std::vector<Point> pts = polygonFromPath(path);
        auto [l, r] = splitPolygonByLine(pts, Point(line.p1()), Point(line.p2()));
        if (!l.empty()) left = pathFromPolygon(l);
        if (!r.empty()) right = pathFromPolygon(r);
        return {left, right};
    }

    static QPainterPath spread(const QPainterPath& path, float amount) { return offset(path, amount); }
    static QPainterPath choke(const QPainterPath& path, float amount) { return offset(path, -amount); }

private:
    static std::vector<Point> douglasPeucker(const std::vector<Point>& points, float epsilon) {
        if (points.size() < 3) return points;
        float dmax = 0;
        size_t index = 0;
        for (size_t i = 1; i < points.size() - 1; ++i) {
            float d = perpendicularDistance(points[i], points.front(), points.back());
            if (d > dmax) { dmax = d; index = i; }
        }
        if (dmax > epsilon) {
            std::vector<Point> left(points.begin(), points.begin() + index + 1);
            std::vector<Point> right(points.begin() + index, points.end());
            auto recLeft = douglasPeucker(left, epsilon);
            auto recRight = douglasPeucker(right, epsilon);
            recLeft.pop_back();
            recLeft.insert(recLeft.end(), recRight.begin(), recRight.end());
            return recLeft;
        }
        return {points.front(), points.back()};
    }

    static float perpendicularDistance(const Point& p, const Point& a, const Point& b) {
        if (a.x == b.x && a.y == b.y) return distance(p, a);
        return std::abs(cross(b - a, p - a)) / length(b - a);
    }

    static std::pair<std::vector<Point>, std::vector<Point>> splitPolygonAtPoint(const std::vector<Point>& poly, const Point& pt, float tolerance) {
        size_t idx = 0;
        float minDist = FLT_MAX;
        for (size_t i = 0; i < poly.size(); ++i) {
            float d = distance(poly[i], pt);
            if (d < minDist) { minDist = d; idx = i; }
        }
        if (minDist > tolerance) return {poly, {}};
        std::vector<Point> left(poly.begin(), poly.begin() + idx + 1);
        std::vector<Point> right(poly.begin() + idx, poly.end());
        return {left, right};
    }

    static std::pair<std::vector<Point>, std::vector<Point>> splitPolygonByLine(const std::vector<Point>& poly, const Point& a, const Point& b) {
        std::vector<Point> left, right;
        if (poly.size() < 3) return {poly, {}};
        Point S = poly.back();
        for (const Point& E : poly) {
            float crossS = cross(b - a, S - a);
            float crossE = cross(b - a, E - a);
            bool Sin = crossS >= 0;
            bool Ein = crossE >= 0;
            if (Ein) {
                if (!Sin) {
                    if (auto I = lineIntersection(S, E, a, b)) left.push_back(*I);
                }
                left.push_back(E);
            } else if (Sin) {
                if (auto I = lineIntersection(S, E, a, b)) right.push_back(*I);
                right.push_back(E);
            }
            S = E;
        }
        return {left, right};
    }

    static QPainterPath roundCorners(const QPolygonF& poly, float radius) {
        QPainterPath path;
        std::vector<Point> pts = polygonFromPath(QPainterPath(poly));
        if (pts.size() < 3) return path;
        size_t n = pts.size();
        for (size_t i = 0; i < n; ++i) {
            Point prev = pts[(i + n - 1) % n];
            Point curr = pts[i];
            Point next = pts[(i + 1) % n];
            Point v1 = normalize(curr - prev);
            Point v2 = normalize(next - curr);
            float angle = std::acos(std::clamp(dot(v1, v2), -1.0f, 1.0f));
            if (angle < EPSILON) { path.lineTo(curr); continue; }
            float tanHalf = std::tan(angle / 2);
            float dist = radius / tanHalf;
            Point p1 = curr - v1 * std::min(dist, length(curr - prev) * 0.5f);
            Point p2 = curr + v2 * std::min(dist, length(next - curr) * 0.5f);
            if (i == 0) path.moveTo(p1);
            else path.lineTo(p1);
            Point center = curr + normalize(v1 + v2) * (radius / std::sin(angle / 2));
            path.arcTo(QRectF(center.x - radius, center.y - radius, radius * 2, radius * 2),
                       std::atan2(-v1.y, -v1.x) * 180 / PI, (PI - angle) * 180 / PI);
        }
        path.closeSubpath();
        return path;
    }

    static QPainterPath connectEndpoints(const QPainterPath& a, const QPainterPath& b, float tolerance) {
        QPolygonF pa = a.toFillPolygon(), pb = b.toFillPolygon();
        if (pa.isEmpty() || pb.isEmpty()) return a.united(b);
        Point aEnd = Point(pa.last()), bStart = Point(pb.first());
        if (distance(aEnd, bStart) <= tolerance) {
            QPainterPath result = a;
            result.addPath(b);
            return result;
        }
        return a.united(b);
    }
};

class PathHitTest {
public:
    static bool hitTestPoint(const QPainterPath& path, const QPointF& point, float tolerance = 3.0f) {
        if (path.contains(point)) return true;
        QPainterPathStroker stroker;
        stroker.setWidth(tolerance * 2);
        return stroker.createStroke(path).contains(point);
    }

    static bool hitTestRect(const QPainterPath& path, const QRectF& rect) {
        return path.intersects(rect);
    }

    static std::optional<QPointF> nearestPointOnPath(const QPainterPath& path, const QPointF& point) {
        float minDist = FLT_MAX;
        QPointF nearest;
        for (const auto& poly : path.toSubpathPolygons()) {
            for (size_t i = 0; i < poly.size() - 1; ++i) {
                QPointF a = poly[i], b = poly[i + 1];
                QPointF proj = projectPointOnLine(point, a, b);
                float d = QLineF(point, proj).length();
                if (d < minDist) { minDist = d; nearest = proj; }
            }
        }
        return minDist < FLT_MAX ? std::make_optional(nearest) : std::nullopt;
    }

    static std::optional<size_t> nearestSegment(const QPainterPath& path, const QPointF& point) {
        float minDist = FLT_MAX;
        size_t nearestIdx = 0;
        bool found = false;
        size_t globalIdx = 0;
        for (const auto& poly : path.toSubpathPolygons()) {
            for (size_t i = 0; i < poly.size() - 1; ++i, ++globalIdx) {
                QPointF a = poly[i], b = poly[i + 1];
                QPointF proj = projectPointOnLine(point, a, b);
                float d = QLineF(point, proj).length();
                if (d < minDist) { minDist = d; nearestIdx = globalIdx; found = true; }
            }
        }
        return found ? std::make_optional(nearestIdx) : std::nullopt;
    }

private:
    static QPointF projectPointOnLine(const QPointF& p, const QPointF& a, const QPointF& b) {
        QPointF ap = p - a, ab = b - a;
        float t = QPointF::dotProduct(ap, ab) / QPointF::dotProduct(ab, ab);
        t = std::clamp(t, 0.0f, 1.0f);
        return a + ab * t;
    }
};

class PathBuilder {
public:
    PathBuilder() = default;

    PathBuilder& moveTo(const QPointF& p) { m_current = p; m_path.moveTo(p); m_started = true; return *this; }
    PathBuilder& lineTo(const QPointF& p) { m_path.lineTo(p); m_current = p; return *this; }
    PathBuilder& curveTo(const QPointF& c1, const QPointF& c2, const QPointF& end) { m_path.cubicTo(c1, c2, end); m_current = end; return *this; }
    PathBuilder& quadTo(const QPointF& c, const QPointF& end) { m_path.quadTo(c, end); m_current = end; return *this; }
    PathBuilder& close() { m_path.closeSubpath(); m_started = false; return *this; }
    PathBuilder& arcTo(const QRectF& rect, float startAngle, float sweepAngle) { m_path.arcTo(rect, startAngle, sweepAngle); return *this; }
    PathBuilder& ellipse(const QRectF& rect) { m_path.addEllipse(rect); return *this; }
    PathBuilder& rect(const QRectF& rect) { m_path.addRect(rect); return *this; }
    PathBuilder& roundedRect(const QRectF& rect, float xRadius, float yRadius) { m_path.addRoundedRect(rect, xRadius, yRadius); return *this; }

    PathBuilder& addPath(const QPainterPath& path) { m_path.addPath(path); return *this; }

    QPainterPath path() const { return m_path; }
    QPainterPath takePath() { QPainterPath p = m_path; m_path = QPainterPath(); m_started = false; return p; }

    void clear() { m_path = QPainterPath(); m_started = false; }

private:
    QPainterPath m_path;
    QPointF m_current;
    bool m_started = false;
};

} // namespace Geometry
} // namespace ArtPlus