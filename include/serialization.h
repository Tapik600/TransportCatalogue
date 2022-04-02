#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <transport_catalogue.pb.h>

namespace serialize {

class Serializer {
    using ProtoStops = google::protobuf::RepeatedPtrField<proto::TransportCatalogue_Stop>;

  public:
    Serializer() = default;
    explicit Serializer(const std::string &filename) : filename_(filename) {}

    bool Save();
    bool Load();

    void Serialize(const tc::TransportCatalogue &,
                   const renderer::MapRenderer &,
                   const router::TransportRouter &);

    tc::TransportCatalogue GetTransportCatalogue();
    const renderer::RendererSettings GetRendererSettings();
    const router::Graph GetRouterGraph();
    const router::EdgesInfo GetRouterEdgesInfo();
    const router::StopVertexes GetRouterVertexes();
    const router::Router::RoutesInternalData GetRouterInternalData();

  private:
    const proto::TransportCatalogue
    SerializeTransportCatalogue(const tc::TransportCatalogue &);
    void SerializeStops(proto::TransportCatalogue &, const domain::StopPtrSet &);
    void SerializeBuses(proto::TransportCatalogue &, const domain::BusPtrSet &);
    void SerializeDistances(proto::TransportCatalogue &, const tc::StopsDist &);

    const proto::TransportRouter SerializeTransportRouter(const router::TransportRouter &);
    void SerializeGraph(proto::TransportRouter &, const router::TransportRouter &);
    void SerializeRouteInternalData(proto::TransportRouter &, const router::TransportRouter &);
    void SerializeRouteInfo(proto::TransportRouter &, const router::TransportRouter &);
    void SerializeVertexes(proto::TransportRouter &, const router::TransportRouter &);

    const proto::RenderSettings SerializeRenderSettings(const renderer::RendererSettings &);

    void DeserializeStops(tc::TransportCatalogue &);
    void DeserializeBuses(tc::TransportCatalogue &);
    void DeserializeDistances(tc::TransportCatalogue &);
    svg::Color DeserializeColor(const proto::Color &);

  private:
    const std::string filename_;
    proto::DataBase db_;
    std::unordered_map<std::string_view, size_t> stop_to_id_;
    std::unordered_map<std::string_view, size_t> bus_to_id_;
};

} // namespace serialize
