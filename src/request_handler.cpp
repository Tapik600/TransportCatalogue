#include "request_handler.h"

#include <sstream>

namespace tc {
using namespace domain;

std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view &bus_name) const {
    return db_.GetBusStat(bus_name);
}

const std::set<BusPtr, detail::BusPtrComparator> *
RequestHandler::GetBusesByStop(const std::string_view &stop_name) const {
    return db_.GetBusesByStop(stop_name);
}

std::string RequestHandler::RenderMap() const {
    std::ostringstream ost;
    renderer_.RenderMap().Render(ost);
    return ost.str();
}

bool RequestHandler::IsStopInCatalogue(const std::string_view &stop_name) const {
    return db_.SearchStop(stop_name) != nullptr;
}

std::optional<router::RouteInfo>
RequestHandler::GetRouteInfo(const std::string_view from, const std::string_view to) const {
    StopPtr from_stop = db_.SearchStop(from);
    StopPtr to_stop = db_.SearchStop(to);
    if (from_stop != nullptr && to_stop != nullptr) {
        return router_.GetRouteInfo(from_stop->name, to_stop->name);
    }
    return {};
}

} // namespace tc