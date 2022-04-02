#include "transport_router.h"

namespace router {

TransportRouter::TransportRouter(const tc::TransportCatalogue &catalogue,
                                 const RoutingSettings &settings)
    : catalogue_(catalogue), settings_(settings) {

    InitializeVertexes();
    InitializeEdges();

    router_ = std::make_unique<Router>(graph_);
}

TransportRouter::TransportRouter(const tc::TransportCatalogue &catalogue,
                                 const StopVertexes &vertex_ids,
                                 const EdgesInfo &edges_info,
                                 const Graph &graph,
                                 const Router::RoutesInternalData &internal_data)
    : catalogue_(catalogue), stops_vertex_ids_(vertex_ids), edges_info_(edges_info),
      graph_(graph) {

    router_ = std::make_unique<Router>(graph_, internal_data);
}

std::optional<RouteInfo> TransportRouter::GetRouteInfo(std::string_view from,
                                                       std::string_view to) const {
    auto route =
        router_->BuildRoute(stops_vertex_ids_.at(from).in, stops_vertex_ids_.at(to).in);
    if (!route) {
        return {};
    }
    RouteInfo route_info;
    route_info.first = route->weight;
    auto &route_items = route_info.second;

    route_items.reserve(route->edges.size());
    for (const auto &edge_id : route->edges) {
        route_items.push_back(edges_info_.at(edge_id));
    }

    return route_info;
}

void TransportRouter::InitializeVertexes() {
    size_t vertex_id{};
    Time weight = settings_.bus_wait_time;
    domain::StopPtrSet stops = catalogue_.GetStops();

    graph_ = Graph(stops.size() * 2);
    edges_info_.reserve(stops.size() * 2);

    for (const auto &stop : stops) {
        auto &vertex_ids = stops_vertex_ids_[stop->name];
        vertex_ids.in = vertex_id++;
        vertex_ids.out = vertex_id++;

        const graph::EdgeId edge_id = graph_.AddEdge({vertex_ids.in, vertex_ids.out, weight});
        edges_info_.insert({edge_id, WaitEdgeInfo{stop->name, weight}});
    }
}

void TransportRouter::InitializeEdges() {
    for (const auto &bus : catalogue_.GetBuses()) {
        const auto &bus_stops = bus->route;

        for (size_t idx_from = 0; idx_from < bus_stops.size() - 1; ++idx_from) {
            VertexIds vertex_from = stops_vertex_ids_.at(bus_stops[idx_from]->name);
            size_t idx_prev = idx_from;

            int span_count{};
            double dist{};

            for (size_t idx_to = idx_from + 1; idx_to < bus_stops.size(); ++idx_to) {
                VertexIds vertex_to = stops_vertex_ids_.at(bus_stops[idx_to]->name);
                Time weight{};

                if (bus_stops[idx_from] != bus_stops[idx_to]) {
                    dist += catalogue_.GetDistanceBetweenStops(bus_stops[idx_prev],
                                                               bus_stops[idx_to]);
                    weight = (dist / settings_.bus_velocity) * TO_MINUTES;

                    const graph::EdgeId edge_id =
                        graph_.AddEdge({vertex_from.out, vertex_to.in, weight});
                    edges_info_.insert(
                        {edge_id, BusEdgeInfo{bus->name, ++span_count, weight}});
                }

                idx_prev = idx_to;
            }
        }
    }
}

} // namespace router