#pragma once

#include "domain.h"
#include "geo.h"

#include <svg.h>

namespace renderer {

struct RendererSettings {
    double width = 0;
    double height = 0;
    double padding = 0;
    double stop_radius = 0;
    double line_width = 0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset = {0.0, 0.0};
    svg::Point stop_label_offset = {0.0, 0.0};
    int stop_label_font_size = 0;
    svg::Color underlayer_color;
    double underlayer_width = 0;
    std::vector<svg::Color> color_palette = {};
};

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
  public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin,
                    PointInputIt points_end,
                    double max_width,
                    double max_height,
                    double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });

        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });

        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const {
        return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
    }

  private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {

  public:
    MapRenderer() = default;
    MapRenderer(const RendererSettings &settings, const domain::BusPtrSet &buses);

    svg::Document RenderMap() const;
    
    const RendererSettings& GetSettings() const {
        return settings_;
    };

  private:
    void DrawRouteLine(svg::Document &) const;
    void DrawBusLables(svg::Document &) const;
    void DrawStopCircles(svg::Document &) const;
    void DrawStopLables(svg::Document &) const;

  private:
    const RendererSettings settings_;
    std::unique_ptr<SphereProjector> projector_;
    const domain::BusPtrSet buses_;
    domain::StopPtrSet stops_;
};

} // namespace renderer