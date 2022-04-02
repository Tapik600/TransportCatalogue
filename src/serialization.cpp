#include "serialization.h"

#include <fstream>

namespace serialize {

bool Serializer::Save() {
    if (filename_.empty()) {
        return false;
    }
    std::ofstream out(filename_, std::ios::binary);
    db_.SerializeToOstream(&out);
    return true;
}

bool Serializer::Load() {
    if (filename_.empty()) {
        return false;
    }
    std::ifstream in(filename_, std::ios::binary);
    db_.ParseFromIstream(&in);
    return true;
}

void Serializer::Serialize(const tc::TransportCatalogue &catalogue,
                           const renderer::MapRenderer &renderer,
                           const router::TransportRouter &router) {

    *db_.mutable_catalogue() = std::move(SerializeTransportCatalogue(catalogue));
    *db_.mutable_router() = std::move(SerializeTransportRouter(router));
    *db_.mutable_render_settings() =
        std::move(SerializeRenderSettings(renderer.GetSettings()));
}

tc::TransportCatalogue Serializer::GetTransportCatalogue() {
    tc::TransportCatalogue catalogue;
    DeserializeStops(catalogue);
    DeserializeBuses(catalogue);
    DeserializeDistances(catalogue);
    return catalogue;
}

const renderer::RendererSettings Serializer::GetRendererSettings() {
    renderer::RendererSettings settings;
    settings.width = db_.render_settings().width();
    settings.height = db_.render_settings().height();
    settings.padding = db_.render_settings().padding();
    settings.stop_radius = db_.render_settings().stop_radius();
    settings.line_width = db_.render_settings().line_width();
    settings.bus_label_font_size = db_.render_settings().bus_label_font_size();
    settings.bus_label_offset = svg::Point(db_.render_settings().bus_label_offset_dx(),
                                           db_.render_settings().bus_label_offset_dy());
    settings.stop_label_font_size = db_.render_settings().stop_label_font_size();
    settings.stop_label_offset = svg::Point(db_.render_settings().stop_label_offset_dx(),
                                            db_.render_settings().stop_label_offset_dy());
    settings.underlayer_width = db_.render_settings().underlayer_width();
    settings.underlayer_color = DeserializeColor(db_.render_settings().underlayer_color());

    for (const auto &s_color : db_.render_settings().color_palette()) {
        settings.color_palette.push_back(DeserializeColor(s_color));
    }

    return settings;
}

const router::Graph Serializer::GetRouterGraph() {
    std::vector<graph::Edge<router::Time>> edges;
    edges.reserve(db_.router().graph().edges_size());
    for (const auto &s_edge : db_.router().graph().edges()) {
        edges.push_back({s_edge.from(), s_edge.to(), s_edge.weight()});
    }
    return router::Graph(edges);
}

const router::EdgesInfo Serializer::GetRouterEdgesInfo() {
    router::EdgesInfo edges_info;
    edges_info.reserve(db_.router().edges_info_size());
    int id{};
    for (const auto &s_edge : db_.router().edges_info()) {
        if (s_edge.is_bus_edge()) {
            edges_info[id] =
                router::BusEdgeInfo{db_.catalogue().buses(s_edge.name_id()).name(),
                                    s_edge.span_count(), s_edge.time()};
        } else {
            edges_info[id] = router::WaitEdgeInfo{
                db_.catalogue().stops(s_edge.name_id()).name(), s_edge.time()};
        }
        ++id;
    }
    return edges_info;
}

const router::StopVertexes Serializer::GetRouterVertexes() {
    router::StopVertexes stop_vertex_ids;
    for (const auto &s_stop_vertex : db_.router().stops_vertex_ids()) {
        stop_vertex_ids[db_.catalogue().stops(s_stop_vertex.stop_id()).name()] =
            router::VertexIds{s_stop_vertex.in(), s_stop_vertex.out()};
    }
    return stop_vertex_ids;
}

const router::Router::RoutesInternalData Serializer::GetRouterInternalData() {
    const int vertex_count = db_.router().routes_internal_data_size();
    router::Router::RoutesInternalData internal_data(
        vertex_count,
        std::vector<std::optional<router::Router::RouteInternalData>>(vertex_count));

    for (int row = 0; row < vertex_count; ++row) {
        for (int col = 0; col < vertex_count; ++col) {
            const auto &vertex = db_.router().routes_internal_data(row).row(col);
            if (vertex.is_prev_edge()) {
                internal_data[row][col] =
                    router::Router::RouteInternalData{vertex.weight(), vertex.prev_edge_id()};
            } else {
                internal_data[row][col] =
                    router::Router::RouteInternalData{vertex.weight(), std::nullopt};
            }
        }
    }
    return internal_data;
}

const proto::TransportCatalogue
Serializer::SerializeTransportCatalogue(const tc::TransportCatalogue &catalogue) {
    proto::TransportCatalogue s_catalogue;
    SerializeStops(s_catalogue, catalogue.GetStops());
    SerializeBuses(s_catalogue, catalogue.GetBuses());
    SerializeDistances(s_catalogue, *catalogue.GetDistances());
    return s_catalogue;
}

void Serializer::SerializeStops(proto::TransportCatalogue &s_catalogue,
                                const domain::StopPtrSet &stops) {
    size_t stop_id = 0;
    for (const auto &stop : stops) {
        proto::TransportCatalogue::Stop s_stop;
        s_stop.set_name(stop->name);
        s_stop.set_coordinates_lat(stop->coordinates.lat);
        s_stop.set_coordinates_lng(stop->coordinates.lng);

        *s_catalogue.add_stops() = std::move(s_stop);
        stop_to_id_[stop->name] = stop_id++;
    }
}

void Serializer::SerializeBuses(proto::TransportCatalogue &s_catalogue,
                                const domain::BusPtrSet &buses) {
    size_t bus_id = 0;
    for (const auto &bus : buses) {
        proto::TransportCatalogue::Bus s_bus;
        s_bus.set_name(bus->name);
        s_bus.set_is_roundtrip(bus->is_roundtrip);
        s_bus.set_final_stop(stop_to_id_.at(bus->final_stop->name));

        for (const auto &stop : bus->route) {
            s_bus.add_route(stop_to_id_.at(stop->name));
        }
        *s_catalogue.add_buses() = std::move(s_bus);
        bus_to_id_[bus->name] = bus_id++;
    }
}

void Serializer::SerializeDistances(proto::TransportCatalogue &s_catalogue,
                                    const tc::StopsDist &distances) {
    for (const auto &[stops, dist] : distances) {
        proto::TransportCatalogue::DistanceBetweenStops s_dist;
        s_dist.set_from_stop_id(stop_to_id_.at(stops.first->name));
        s_dist.set_to_stop_id(stop_to_id_.at(stops.second->name));
        s_dist.set_dist(dist);

        *s_catalogue.add_distance() = std::move(s_dist);
    }
}

const proto::TransportRouter
Serializer::SerializeTransportRouter(const router::TransportRouter &router) {
    proto::TransportRouter s_router;
    SerializeGraph(s_router, router);
    SerializeRouteInternalData(s_router, router);
    SerializeRouteInfo(s_router, router);
    SerializeVertexes(s_router, router);
    return s_router;
}

void Serializer::SerializeGraph(proto::TransportRouter &s_router,
                                const router::TransportRouter &router) {
    proto::Graph graph;
    for (const auto &edge : router.GetGraph().GetEdges()) {
        proto::Graph::Edge s_edge;
        s_edge.set_from(edge.from);
        s_edge.set_to(edge.to);
        s_edge.set_weight(edge.weight);
        *graph.add_edges() = std::move(s_edge);
    }
    *s_router.mutable_graph() = std::move(graph);
}

void Serializer::SerializeRouteInternalData(proto::TransportRouter &s_router,
                                            const router::TransportRouter &router) {
    const auto &internal_data = router.GetRoutesInternalData();
    for (const auto &internal_data_row : internal_data) {
        proto::TransportRouter::RouteInternalDataRow s_internal_data_row;
        for (const auto &internal_data : internal_data_row) {
            proto::TransportRouter::RouteInternalData s_internal_data;
            s_internal_data.set_weight(internal_data->weight);
            if (internal_data->prev_edge.has_value()) {
                s_internal_data.set_prev_edge_id(internal_data->prev_edge.value());
                s_internal_data.set_is_prev_edge(true);
            }
            *s_internal_data_row.add_row() = std::move(s_internal_data);
        }
        *s_router.add_routes_internal_data() = std::move(s_internal_data_row);
    }
}

const std::vector<router::EdgeInfo> EdgesInfoToVector(const router::EdgesInfo &edges_info) {
    std::vector<router::EdgeInfo> edge_info_vec(edges_info.size());
    for (const auto &[id, edge] : edges_info) {
        edge_info_vec[id] = edge;
    }
    return edge_info_vec;
}

void Serializer::SerializeRouteInfo(proto::TransportRouter &s_router,
                                    const router::TransportRouter &router) {

    for (const auto &info : EdgesInfoToVector(router.GetEdgesInfo())) {
        proto::TransportRouter::EdgeInfo s_info;
        if (std::holds_alternative<router::WaitEdgeInfo>(info)) {
            s_info.set_name_id(stop_to_id_.at(std::get<router::WaitEdgeInfo>(info).name));
            s_info.set_time(std::get<router::WaitEdgeInfo>(info).time);

        } else if (std::holds_alternative<router::BusEdgeInfo>(info)) {
            s_info.set_name_id(bus_to_id_.at(std::get<router::BusEdgeInfo>(info).name));
            s_info.set_time(std::get<router::BusEdgeInfo>(info).time);
            s_info.set_span_count(std::get<router::BusEdgeInfo>(info).span_count);
            s_info.set_is_bus_edge(true);
        }
        *s_router.add_edges_info() = std::move(s_info);
    }
}

void Serializer::SerializeVertexes(proto::TransportRouter &s_router,
                                   const router::TransportRouter &router) {
    for (const auto &[name, vertex] : router.GetStopsVertexIds()) {
        proto::TransportRouter::StopVertexes s_stop_vertex;
        s_stop_vertex.set_stop_id(stop_to_id_.at(name));
        s_stop_vertex.set_in(vertex.in);
        s_stop_vertex.set_out(vertex.out);
        *s_router.add_stops_vertex_ids() = std::move(s_stop_vertex);
    }
}

struct GetSerializedColor {
    [[nodiscard]] proto::Color operator()(std::monostate) const {
        return proto::Color{};
    }
    [[nodiscard]] proto::Color operator()(const std::string &color) {
        proto::Color s_color;
        s_color.set_is_rgba(false);
        s_color.set_name(color);
        return s_color;
    }
    [[nodiscard]] proto::Color operator()(const svg::Rgb &color) {
        proto::Color::Rgba rgb;
        rgb.set_red(color.red);
        rgb.set_green(color.green);
        rgb.set_blue(color.blue);

        proto::Color s_color;
        s_color.set_is_rgba(false);
        *s_color.mutable_rgba() = std::move(rgb);

        return s_color;
    }
    [[nodiscard]] proto::Color operator()(const svg::Rgba &color) {
        proto::Color::Rgba rgba;
        rgba.set_red(color.red);
        rgba.set_green(color.green);
        rgba.set_blue(color.blue);
        rgba.set_opacity(color.opacity);

        proto::Color s_color;
        s_color.set_is_rgba(true);
        *s_color.mutable_rgba() = std::move(rgba);

        return s_color;
    }
};

const proto::RenderSettings
Serializer::SerializeRenderSettings(const renderer::RendererSettings &settings) {
    proto::RenderSettings s_settings;
    s_settings.set_width(settings.width);
    s_settings.set_height(settings.height);
    s_settings.set_padding(settings.padding);
    s_settings.set_stop_radius(settings.stop_radius);
    s_settings.set_line_width(settings.line_width);
    s_settings.set_bus_label_font_size(settings.bus_label_font_size);
    s_settings.set_bus_label_offset_dx(settings.bus_label_offset.x);
    s_settings.set_bus_label_offset_dy(settings.bus_label_offset.y);
    s_settings.set_stop_label_offset_dx(settings.stop_label_offset.x);
    s_settings.set_stop_label_offset_dy(settings.stop_label_offset.y);
    s_settings.set_stop_label_font_size(settings.stop_label_font_size);
    s_settings.set_underlayer_width(settings.underlayer_width);
    *s_settings.mutable_underlayer_color() =
        std::move(std::visit(GetSerializedColor{}, settings.underlayer_color));

    for (const auto &color : settings.color_palette) {
        *s_settings.add_color_palette() = std::move(std::visit(GetSerializedColor{}, color));
    }
    return s_settings;
}

void Serializer::DeserializeStops(tc::TransportCatalogue &catalogue) {
    for (const auto &s_stop : db_.catalogue().stops()) {
        catalogue.AddStop({s_stop.name(), geo::Coordinates{s_stop.coordinates_lat(),
                                                           s_stop.coordinates_lng()}});
    }
}

void Serializer::DeserializeBuses(tc::TransportCatalogue &catalogue) {
    for (const auto &s_bus : db_.catalogue().buses()) {
        domain::Bus bus;
        bus.name = s_bus.name();
        bus.is_roundtrip = s_bus.is_roundtrip();

        bus.route.reserve(s_bus.route_size());
        for (const auto &s_stop : s_bus.route()) {
            bus.route.push_back(catalogue.SearchStop(db_.catalogue().stops(s_stop).name()));
        }
        bus.final_stop =
            catalogue.SearchStop(db_.catalogue().stops(s_bus.final_stop()).name());
        catalogue.AddBus(bus);
    }
}

void Serializer::DeserializeDistances(tc::TransportCatalogue &catalogue) {
    for (const auto &dist : db_.catalogue().distance()) {
        catalogue.SetDistanceBetweenStops(db_.catalogue().stops(dist.from_stop_id()).name(),
                                          db_.catalogue().stops(dist.to_stop_id()).name(),
                                          dist.dist());
    }
}

svg::Color Serializer::DeserializeColor(const proto::Color &s_color) {
    svg::Color color;
    if (!s_color.name().empty()) {
        color = s_color.name();
    } else if (s_color.is_rgba()) {
        color = svg::Rgba{s_color.rgba().red(), s_color.rgba().green(), s_color.rgba().blue(),
                          s_color.rgba().opacity()};
    } else {
        color = svg::Rgb{s_color.rgba().red(), s_color.rgba().green(), s_color.rgba().blue()};
    }
    return color;
}

} // namespace serialize
