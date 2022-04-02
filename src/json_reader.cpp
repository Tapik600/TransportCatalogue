#include "json_reader.h"

#include <variant>

namespace json::reader {
using namespace std::literals;

void JsonReader::ReadRequests(std::istream &input) {
    const Document doc = json::Load(input);
    const auto &requests = doc.GetRoot().AsDict();

    if (requests.count("base_requests"s)) {
        base_requests_ = requests.at("base_requests"s).AsArray();
    }
    if (requests.count("stat_requests"s)) {
        stat_requests_ = requests.at("stat_requests"s).AsArray();
    }
    if (requests.count("render_settings"s)) {
        render_settings_ = requests.at("render_settings"s).AsDict();
    }
    if (requests.count("routing_settings"s)) {
        routing_settings_ = requests.at("routing_settings"s).AsDict();
    }
    if (requests.count("serialization_settings"s)) {
        serialization_settings_ = requests.at("serialization_settings"s).AsDict();
    }
}

void JsonReader::AddStops(const std::vector<json::Dict> &requests,
                          tc::TransportCatalogue &db) const {
    using namespace std;
    vector<pair<string_view, Dict>> stop_distance;

    for (const auto &req : requests) {
        domain::Stop stop{
            req.at("name"s).AsString(),
            geo::Coordinates{req.at("latitude"s).AsDouble(), req.at("longitude"s).AsDouble()}};
        db.AddStop(stop);
        stop_distance.push_back(
            {req.at("name"s).AsString(), req.at("road_distances"s).AsDict()});
    }
    for (const auto &[from, dict] : stop_distance) {
        for (const auto &[to, distance] : dict) {
            db.SetDistanceBetweenStops(from, to, distance.AsDouble());
        }
    }
}

void JsonReader::AddBuses(const std::vector<Dict> &requests,
                          tc::TransportCatalogue &db) const {
    using namespace std;
    for (const auto &req : requests) {
        domain::Bus bus;
        bus.name = req.at("name"s).AsString();
        bus.is_roundtrip = req.at("is_roundtrip"s).AsBool();

        const auto &route_node = req.at("stops"s).AsArray();

        bus.route.reserve(route_node.size());
        for (auto &stop : route_node) {
            bus.route.push_back(db.SearchStop(stop.AsString()));
        }
        bus.final_stop = bus.route.back();
        if (!bus.is_roundtrip) {
            bus.route.reserve(route_node.size() * 2);
            bus.route.insert(bus.route.end(), ++bus.route.rbegin(), bus.route.rend());
        }

        db.AddBus(bus);
    }
}

void JsonReader::ExecuteBaseRequest(tc::TransportCatalogue &db) const {
    using namespace std;
    vector<Dict> add_stop_requests;
    vector<Dict> add_bus_requests;

    for (const auto &base_request : base_requests_) {
        const auto &request = base_request.AsDict();
        const auto &type = request.at("type"s).AsString();

        if (type == "Stop"s) {
            add_stop_requests.push_back(request);
        } else if (type == "Bus"s) {
            add_bus_requests.push_back(request);
        }
    }

    AddStops(add_stop_requests, db);
    AddBuses(add_bus_requests, db);
}

void JsonReader::ExecuteStatRequest(std::ostream &out,
                                    const tc::RequestHandler &handler) const {
    json::Array response;

    for (const auto &node : stat_requests_) {
        response.reserve(stat_requests_.size());

        const auto &request = node.AsDict();
        const auto &type = request.at("type"s).AsString();

        if (type == "Stop"s) {
            response.push_back(GetStopStat(request, handler));
        } else if (type == "Bus"s) {
            response.push_back(GetBusStat(request, handler));
        } else if (type == "Map"s) {
            response.push_back(GetMap(request, handler));
        } else if (type == "Route"s) {
            response.push_back(GetRoute(request, handler));
        }
    }
    json::Print(json::Document(json::Node(response)), out);
}

json::Node JsonReader::GetStopStat(const json::Dict &request,
                                   const tc::RequestHandler &handler) const {
    const int &id = request.at("id"s).AsInt();
    const std::string &stop_name = request.at("name"s).AsString();

    if (!handler.IsStopInCatalogue(stop_name)) {
        return json::Builder{}
            .StartDict()
            .Key("request_id"s)
            .Value(id)
            .Key("error_message"s)
            .Value("not found"s)
            .EndDict()
            .Build();
    }

    auto buses = handler.GetBusesByStop(stop_name);
    json::Array buses_response;

    if (buses) {
        buses_response.reserve(buses->size());
        for (const auto &bus : *buses) {
            buses_response.push_back(json::Node(bus->name));
        }
    }

    return json::Builder{}
        .StartDict()
        .Key("buses"s)
        .Value(std::move(buses_response))
        .Key("request_id"s)
        .Value(id)
        .EndDict()
        .Build();
}

json::Node JsonReader::GetBusStat(const json::Dict &request,
                                  const tc::RequestHandler &handler) const {
    const auto &id = request.at("id"s).AsInt();
    const auto &bus_name = request.at("name"s).AsString();

    const auto &stat = handler.GetBusStat(bus_name);

    if (stat.has_value()) {
        return json::Builder{}
            .StartDict()
            .Key("request_id"s)
            .Value(id)
            .Key("curvature"s)
            .Value(stat->curvature)
            .Key("unique_stop_count"s)
            .Value(static_cast<int>(stat->unique_stops))
            .Key("stop_count"s)
            .Value(static_cast<int>(stat->stops_on_route))
            .Key("route_length"s)
            .Value(stat->route_length)
            .EndDict()
            .Build();
    }

    return json::Builder{}
        .StartDict()
        .Key("request_id"s)
        .Value(id)
        .Key("error_message"s)
        .Value("not found"s)
        .EndDict()
        .Build();
}

json::Node JsonReader::GetMap(const json::Dict &request,
                              const tc::RequestHandler &handler) const {
    return json::Builder{}
        .StartDict()
        .Key("request_id"s)
        .Value(request.at("id"s).AsInt())
        .Key("map"s)
        .Value(handler.RenderMap())
        .EndDict()
        .Build();
}

struct BuildRouteItem {
    [[nodiscard]] json::Node operator()(const router::WaitEdgeInfo &edge) {
        return json::Builder{}
            .StartDict()
            .Key("type"s)
            .Value("Wait"s)
            .Key("time"s)
            .Value(edge.time)
            .Key("stop_name"s)
            .Value(std::string(edge.name))
            .EndDict()
            .Build();
    }
    [[nodiscard]] json::Node operator()(const router::BusEdgeInfo &edge) {
        return json::Builder{}
            .StartDict()
            .Key("type"s)
            .Value("Bus"s)
            .Key("time"s)
            .Value(edge.time)
            .Key("bus"s)
            .Value(std::string(edge.name))
            .Key("span_count"s)
            .Value(edge.span_count)
            .EndDict()
            .Build();
    }
};

json::Node JsonReader::GetRoute(const json::Dict &request,
                                const tc::RequestHandler &handler) const {
    const auto &id = request.at("id"s).AsInt();
    const auto &route_info =
        handler.GetRouteInfo(request.at("from"s).AsString(), request.at("to"s).AsString());
    if (route_info.has_value()) {
        json::Array items;
        for (const auto &item : route_info->second) {
            items.push_back(std::visit(BuildRouteItem{}, item));
        }
        return json::Builder{}
            .StartDict()
            .Key("request_id"s)
            .Value(id)
            .Key("total_time"s)
            .Value(route_info->first)
            .Key("items"s)
            .Value(items)
            .EndDict()
            .Build();
    }
    return json::Builder{}
        .StartDict()
        .Key("request_id"s)
        .Value(id)
        .Key("error_message"s)
        .Value("not found"s)
        .EndDict()
        .Build();
}

const renderer::RendererSettings JsonReader::GetRendererSettings() {
    if (render_settings_.empty()) {
        return {};
    }

    renderer::RendererSettings settings;
    settings.width = render_settings_.at("width"s).AsDouble();
    settings.height = render_settings_.at("height"s).AsDouble();
    settings.padding = render_settings_.at("padding"s).AsDouble();
    settings.line_width = render_settings_.at("line_width"s).AsDouble();
    settings.stop_radius = render_settings_.at("stop_radius"s).AsDouble();
    settings.bus_label_font_size = render_settings_.at("bus_label_font_size"s).AsInt();

    const json::Array &bus_label_offset = render_settings_.at("bus_label_offset"s).AsArray();

    settings.bus_label_offset =
        svg::Point(bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble());

    settings.stop_label_font_size = render_settings_.at("stop_label_font_size"s).AsInt();

    const json::Array &stop_label_offset = render_settings_.at("stop_label_offset"s).AsArray();

    settings.stop_label_offset =
        svg::Point(stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble());

    settings.underlayer_color = ParseColor(render_settings_.at("underlayer_color"s));
    settings.underlayer_width = render_settings_.at("underlayer_width"s).AsDouble();

    const auto &array_color = render_settings_.at("color_palette"s).AsArray();
    settings.color_palette.reserve(array_color.size());
    for (const auto &color : array_color) {
        settings.color_palette.push_back(ParseColor(color));
    }

    return settings;
}

const std::string JsonReader::GetSerializationSettings() {
    if (serialization_settings_.count("file"s)) {
        return serialization_settings_.at("file"s).AsString();
    }
    return {};
}

const router::RoutingSettings JsonReader::GetRoutingSettings() {
    router::RoutingSettings settings{};
    if (routing_settings_.count("bus_velocity"s) > 0 &&
        routing_settings_.count("bus_wait_time"s) > 0) {

        settings.bus_velocity = routing_settings_.at("bus_velocity"s).AsDouble();
        settings.bus_wait_time = routing_settings_.at("bus_wait_time"s).AsInt();
    }
    return settings;
}

svg::Color JsonReader::ParseColor(const json::Node &node) {
    if (node.IsString()) {
        return svg::Color{node.AsString()};
    }
    if (node.IsArray()) {

        const auto &node_array = node.AsArray();
        size_t size = node_array.size();
        if (size == 4u) {

            return svg::Rgba{static_cast<uint8_t>(node_array[0].AsInt()),
                             static_cast<uint8_t>(node_array[1].AsInt()),
                             static_cast<uint8_t>(node_array[2].AsInt()),
                             node_array[3].AsDouble()};
        }
        if (size == 3u) {
            return svg::Rgb{static_cast<uint8_t>(node_array[0].AsInt()),
                            static_cast<uint8_t>(node_array[1].AsInt()),
                            static_cast<uint8_t>(node_array[2].AsInt())};
        }
    } else {
        throw std::logic_error("unknown node"s);
    }
    return svg::Color{};
}

} // namespace json::reader
