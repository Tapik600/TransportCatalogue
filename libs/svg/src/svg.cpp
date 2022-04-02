#include "svg.h"

namespace svg {

using namespace std::literals;

std::ostream &operator<<(std::ostream &os, Rgb rgb) {
    os << "rgb("sv << unsigned(rgb.red) << ',' << unsigned(rgb.green) << ','
       << unsigned(rgb.blue) << ')';
    return os;
}

std::ostream &operator<<(std::ostream &os, Rgba rgba) {
    os << "rgba("sv << unsigned(rgba.red) << ',' << unsigned(rgba.green) << ','
       << unsigned(rgba.blue) << ',' << rgba.opacity << ')';
    return os;
}

std::ostream &operator<<(std::ostream &os, svg::StrokeLineCap line_cap) {
    switch (line_cap) {
    case svg::StrokeLineCap::BUTT:
        os << "butt"sv;
        break;

    case svg::StrokeLineCap::ROUND:
        os << "round"sv;
        break;

    case svg::StrokeLineCap::SQUARE:
        os << "square"sv;
        break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, svg::StrokeLineJoin line_join) {
    switch (line_join) {
    case svg::StrokeLineJoin::ARCS:
        os << "arcs"sv;
        break;

    case svg::StrokeLineJoin::BEVEL:
        os << "bevel"sv;
        break;

    case svg::StrokeLineJoin::MITER:
        os << "miter"sv;
        break;

    case svg::StrokeLineJoin::MITER_CLIP:
        os << "miter-clip"sv;
        break;

    case svg::StrokeLineJoin::ROUND:
        os << "round"sv;
        break;
    }
    return os;
}

namespace detail {
std::size_t ReplaceAll(std::string &inout, std::string_view what, std::string_view with) {
    size_t count{};
    for (std::string::size_type pos{};
         inout.npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count) {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}
} // namespace detail

void Object::Render(const RenderContext &context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle &Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle &Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    this->RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ----------------

Polyline &Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<polyline "sv;
    out << "points=\""sv;
    if (!points_.empty()) {
        std::string_view elem = ""sv;
        for (const auto &p : points_) {
            out << elem << p.x << ',' << p.y;
            elem = " "sv;
        }
    }
    out << "\""sv;
    this->RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text --------------------

Text &Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text &Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text &Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text &Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text &Text::SetData(std::string data) {
    data_ = data;
    return *this;
}

std::string Text::RenderData() const {
    std::string data = data_;

    detail::ReplaceAll(data, "&"sv, "\\&amp;"sv);
    detail::ReplaceAll(data, "\""sv, "\\&quot;"sv);
    detail::ReplaceAll(data, "\'"sv, "\\&apos;"sv);
    detail::ReplaceAll(data, "<"sv, "\\&lt;"sv);
    detail::ReplaceAll(data, ">"sv, "\\&gt;"sv);

    return data;
}

void Text::RenderObject(const RenderContext &context) const {
    auto &out = context.out;
    out << "<text"sv;
    this->RenderAttrs(out);
    out << " x=\""sv << position_.x << "\" y=\""sv << position_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << size_ << "\""sv;
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">"sv;
    out << RenderData();
    out << "</text>"sv;
}

// ---------- Document ----------------

void Document::Render(std::ostream &out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

    for (size_t i = 0; i < objects_.size(); ++i) {
        objects_[i].get()->Render({out, 2, 2});
    }

    out << "</svg>"sv;
}

} // namespace svg