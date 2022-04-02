#include "map_renderer.h"

namespace renderer {

using namespace std::literals;

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

MapRenderer::MapRenderer(const RendererSettings &settings, const domain::BusPtrSet &buses)
    : settings_(std::move(settings)), buses_(std::move(buses)) {

    for (const auto &bus : buses_) {
        stops_.insert(bus->route.begin(), bus->route.end());
    }

    std::vector<geo::Coordinates> points;
    points.reserve(stops_.size());
    for (const auto &stop : stops_) {
        points.push_back(stop->coordinates);
    }

    projector_ = std::make_unique<SphereProjector>(
        points.begin(), points.end(), settings_.width, settings_.height, settings_.padding);
}

svg::Document MapRenderer::RenderMap() const {
    svg::Document map;

    DrawRouteLine(map);
    DrawBusLables(map);
    DrawStopCircles(map);
    DrawStopLables(map);

    return map;
}

void MapRenderer::DrawStopCircles(svg::Document &map) const {
    for (const auto &stop : stops_) {
        map.Add(svg::Circle()
                    .SetCenter(projector_->operator()(stop->coordinates))
                    .SetRadius(settings_.stop_radius)
                    .SetFillColor("white"s));
    }
}

void MapRenderer::DrawRouteLine(svg::Document &map) const {
    size_t color = 0;

    for (const auto &bus : buses_) {
        if (bus->route.size() < 2u)
            continue;

        svg::Polyline line = svg::Polyline()
                                 .SetFillColor({svg::NoneColor})
                                 .SetStrokeColor(settings_.color_palette[color])
                                 .SetStrokeWidth(settings_.line_width)
                                 .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                                 .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (const auto &stop : bus->route) {
            line.AddPoint(projector_->operator()(stop->coordinates));
        }

        color++;
        if (color == settings_.color_palette.size()) {
            color = 0;
        }

        map.Add(line);
    }
}

void MapRenderer::DrawStopLables(svg::Document &map) const {
    for (const auto &stop : stops_) {

        map.Add(svg::Text() // background
                    .SetPosition(projector_->operator()(stop->coordinates))
                    .SetFontFamily("Verdana"s)
                    .SetData(stop->name)
                    .SetFontSize(settings_.stop_label_font_size)
                    .SetOffset(settings_.stop_label_offset)
                    .SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color)
                    .SetStrokeWidth(settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                    .SetFontWeight({}));

        map.Add(svg::Text() // lable
                    .SetFillColor("black"s)
                    .SetFontFamily("Verdana"s)
                    .SetData(stop->name)
                    .SetPosition(projector_->operator()(stop->coordinates))
                    .SetOffset(settings_.stop_label_offset)
                    .SetFontSize(settings_.stop_label_font_size)
                    .SetFontWeight({}));
    }
}

void MapRenderer::DrawBusLables(svg::Document &map) const {
    size_t color = 0;

    const auto &GetBackground = [&](const std::string &bus_name,
                                    const geo::Coordinates &coord) {
        return svg::Text()
            .SetPosition(projector_->operator()(coord))
            .SetData(bus_name)
            .SetFontWeight("bold"s)
            .SetFontFamily("Verdana"s)
            .SetFontSize(settings_.bus_label_font_size)
            .SetOffset(settings_.bus_label_offset)
            .SetFillColor(settings_.underlayer_color)
            .SetStrokeColor(settings_.underlayer_color)
            .SetStrokeWidth(settings_.underlayer_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    };

    const auto &GetLableText = [&](const std::string &bus_name,
                                   const geo::Coordinates &coord) {
        return svg::Text()
            .SetPosition(projector_->operator()(coord))
            .SetData(bus_name)
            .SetFontWeight("bold"s)
            .SetFontFamily("Verdana"s)
            .SetFontSize(settings_.bus_label_font_size)
            .SetOffset(settings_.bus_label_offset)
            .SetFillColor(settings_.color_palette[color]);
    };

    for (const auto &bus : buses_) {
        const auto route_size = bus->route.size();
        if (route_size == 0) {
            continue;
        }

        map.Add(GetBackground(bus->name, bus->route[0]->coordinates));
        map.Add(GetLableText(bus->name, bus->route[0]->coordinates));

        if (!bus->is_roundtrip && bus->route[0].get() != bus->final_stop.get()) {
            map.Add(GetBackground(bus->name, bus->final_stop->coordinates));
            map.Add(GetLableText(bus->name, bus->final_stop->coordinates));
        }

        color++;
        if (color == settings_.color_palette.size()) {
            color = 0;
        }
    }
}

} // namespace renderer