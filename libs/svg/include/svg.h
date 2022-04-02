#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {

struct Rgb {
    Rgb() = default;
    Rgb(unsigned int r, unsigned int g, unsigned int b) : red(r), green(g), blue(b) {}

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};
std::ostream &operator<<(std::ostream &os, Rgb rgb);

struct Rgba {
    Rgba() = default;
    Rgba(unsigned int r, unsigned int g, unsigned int b, double a)
        : red(r), green(g), blue(b), opacity(a) {}

    uint8_t red = 0u;
    uint8_t green = 0u;
    uint8_t blue = 0u;
    double opacity = 1.0f;
};
std::ostream &operator<<(std::ostream &os, Rgba rgba);

struct ColorPrinter {
    std::ostream &out;

    void operator()(std::monostate) const {}
    void operator()(std::string color) const {
        out << color;
    }
    void operator()(Rgb color) const {
        out << color;
    }
    void operator()(Rgba color) const {
        out << color;
    }
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const Color NoneColor{"none"};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream &operator<<(std::ostream &os, svg::StrokeLineCap line_cap);
std::ostream &operator<<(std::ostream &os, svg::StrokeLineJoin line_join);

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
    double x = 0;
    double y = 0;

    Point operator+(const Point &p) {
        return {x + p.x, y + p.y};
    }
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream &out) : out(out) {}

    RenderContext(std::ostream &out, int indent_step, int indent = 0)
        : out(out), indent_step(indent_step), indent(indent) {}

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream &out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
  public:
    void Render(const RenderContext &context) const;

    virtual ~Object() = default;

  private:
    virtual void RenderObject(const RenderContext &context) const = 0;
};

template <typename Owner>
class PathProps {
  public:
    Owner &SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner &SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner &SetStrokeWidth(double width) {
        width_ = width;
        return AsOwner();
    };
    Owner &SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = line_cap;
        return AsOwner();
    };
    Owner &SetStrokeLineJoin(StrokeLineJoin line_join) {
        StrokeLineJoin_ = line_join;
        return AsOwner();
    };

  protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream &out) const {
        using namespace std::literals;

        if (fill_color_ && !std::holds_alternative<std::monostate>(*fill_color_)) {
            out << " fill=\""sv;
            visit(ColorPrinter{out}, *fill_color_);
            out << "\""sv;
        }

        if (stroke_color_ && !std::holds_alternative<std::monostate>(*stroke_color_)) {
            out << " stroke=\""sv;
            visit(ColorPrinter{out}, *stroke_color_);
            out << "\""sv;
        }
        if (width_) {
            out << " stroke-width=\""sv << *width_ << "\""sv;
        }
        if (line_cap_) {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (StrokeLineJoin_) {
            out << " stroke-linejoin=\""sv << *StrokeLineJoin_ << "\""sv;
        }
    }

  private:
    Owner &AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner &>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> StrokeLineJoin_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
  public:
    Circle &SetCenter(Point center);
    Circle &SetRadius(double radius);

  private:
    void RenderObject(const RenderContext &context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
  public:
    Polyline() = default;
    // Добавляет очередную вершину к ломаной линии
    Polyline &AddPoint(Point point);

  private:
    void RenderObject(const RenderContext &context) const override;

    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
  public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text &SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text &SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text &SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text &SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text &SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text &SetData(std::string data);

    // Прочие данные и методы, необходимые для реализации элемента <text>
  private:
    void RenderObject(const RenderContext &context) const override;
    std::string RenderData() const;

    Point position_;
    Point offset_;
    uint32_t size_ = 1;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

namespace detail {
std::size_t ReplaceAll(std::string &inout, std::string_view what, std::string_view with);
}

// ObjectContainer задаёт интерфейс для доступа к контейнеру SVG-объектов
class ObjectContainer {
  public:
    virtual void AddPtr(std::unique_ptr<Object> &&) = 0;

    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::move(std::make_unique<Obj>(std::move(obj))));
    }
};

class Drawable {
  public:
    virtual ~Drawable() = default;

    virtual void Draw(ObjectContainer &container) const = 0;
};

class Document : public ObjectContainer {
  public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object> &&obj) {
        objects_.emplace_back(std::move(obj));
    };

    // Выводит в ostream svg-представление документа
    void Render(std::ostream &out) const;

    // Прочие методы и данные, необходимые для реализации класса Document
  private:
    std::vector<std::unique_ptr<Object>> objects_;
};

} // namespace svg