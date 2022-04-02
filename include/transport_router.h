#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <variant>

namespace router {

struct RoutingSettings {
    int bus_wait_time = 1;
    double bus_velocity = 1;
};

struct WaitEdgeInfo {
    std::string_view name;
    double time{};
};

struct BusEdgeInfo {
    std::string_view name;
    int span_count{};
    double time{};
};

struct VertexIds {
    graph::VertexId in{};
    graph::VertexId out{};
};

using Time = double;
using Router = graph::Router<Time>;
using Graph = graph::DirectedWeightedGraph<Time>;
using EdgeInfo = std::variant<WaitEdgeInfo, BusEdgeInfo>;
using RouteInfo = std::pair<double, std::vector<EdgeInfo>>;
using EdgesInfo = std::unordered_map<graph::EdgeId, EdgeInfo>;
using StopVertexes = std::unordered_map<std::string_view, VertexIds>;

class TransportRouter {
  private:
    static constexpr Time TO_MINUTES = (3.6 / 60.0);

  public:
    TransportRouter() = default;
    explicit TransportRouter(const tc::TransportCatalogue &, const RoutingSettings &);

    explicit TransportRouter(const tc::TransportCatalogue &,
                             const StopVertexes &,
                             const EdgesInfo &,
                             const Graph &,
                             const Router::RoutesInternalData &);

    std::optional<RouteInfo> GetRouteInfo(std::string_view from, std::string_view to) const;

    const EdgesInfo &GetEdgesInfo() const {
        return edges_info_;
    }
    const RoutingSettings &GetSettings() const {
        return settings_;
    }
    const Graph &GetGraph() const {
        return graph_;
    }
    const Router::RoutesInternalData &GetRoutesInternalData() const {
        return router_->GetRoutesInternalData();
    }
    const StopVertexes &GetStopsVertexIds() const {
        return stops_vertex_ids_;
    }

  private:
    void InitializeVertexes();
    void InitializeEdges();

  private:
    const tc::TransportCatalogue &catalogue_;
    const RoutingSettings settings_;
    std::unique_ptr<Router> router_;
    StopVertexes stops_vertex_ids_;
    EdgesInfo edges_info_;
    Graph graph_;
};

} // namespace router