#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

namespace tc {

class RequestHandler {
  public:
    RequestHandler(const TransportCatalogue &db,
                   const renderer::MapRenderer &renderer,
                   const router::TransportRouter &router)
        : db_(db), renderer_(renderer), router_(router) {}

    std::optional<domain::BusStat> GetBusStat(const std::string_view &bus_name) const;

    const domain::BusPtrSet *GetBusesByStop(const std::string_view &stop_name) const;

    bool IsStopInCatalogue(const std::string_view &stop_name) const;

    std::string RenderMap() const;

    std::optional<router::RouteInfo> GetRouteInfo(const std::string_view from,
                                                  const std::string_view to) const;

  private:
    const TransportCatalogue &db_;
    const renderer::MapRenderer &renderer_;
    const router::TransportRouter &router_;
};

} // namespace tc
