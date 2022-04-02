#include "transport_catalogue.h"

#include <numeric>

namespace tc {

using namespace domain;

void TransportCatalogue::AddStop(Stop &stop) {
    AddStop(std::make_shared<Stop>(stop));
}
void TransportCatalogue::AddStop(Stop &&stop) {
    AddStop(std::make_shared<Stop>(stop));
}

void TransportCatalogue::AddStop(StopPtr stop) {
    if (name_to_stop_.count(stop->name) == 0) {
        name_to_stop_.insert(std::make_pair(std::string_view(stop->name), stop));
    }
}
void TransportCatalogue::AddBus(Bus &bus) {
    AddBus(std::make_shared<Bus>(bus));
}
void TransportCatalogue::AddBus(Bus &&bus) {
    AddBus(std::make_shared<Bus>(bus));
}

void TransportCatalogue::AddBus(BusPtr bus) {
    if (name_to_bus_.count(bus->name) == 0) {
        name_to_bus_.insert(std::make_pair(std::string_view(bus->name), bus));
        for (auto stop : bus->route) {
            if (IsStopInCatalogue(stop)) {
                stop_to_buses_[std::string_view(stop->name)].insert(bus);
            }
        }
    }
}

StopPtr TransportCatalogue::SearchStop(std::string_view name) const {
    if (name_to_stop_.count(name) > 0) {
        return name_to_stop_.at(name);
    }
    return nullptr;
}

BusPtr TransportCatalogue::SearchBus(std::string_view name) const {
    if (name_to_bus_.count(name) > 0) {
        return name_to_bus_.at(name);
    }
    return nullptr;
}

void TransportCatalogue::SetDistanceBetweenStops(StopPtr from,
                                                 StopPtr to,
                                                 const double distance) {
    if (IsStopInCatalogue(from) && IsStopInCatalogue(to)) {
        stops_to_distance_[{from, to}] = distance;
    }
}

void TransportCatalogue::SetDistanceBetweenStops(const std::string_view &from,
                                                 const std::string_view &to,
                                                 const double distance) {
    StopPtr stop_from = SearchStop(from);
    StopPtr stop_to = SearchStop(to);
    if (stop_from && stop_to) {
        stops_to_distance_[{stop_from, stop_to}] = distance;
    }
}

double TransportCatalogue::GetDistanceBetweenStops(StopPtr from, StopPtr to) const {
    if (stops_to_distance_.count({from, to}) > 0) {
        return stops_to_distance_.at({from, to});
    }
    if (stops_to_distance_.count({to, from}) > 0) {
        return stops_to_distance_.at({to, from});
    }

    return 0.0;
}

const StopsDist* TransportCatalogue::GetDistances() const
{
    return &stops_to_distance_;
}

const std::optional<BusStat>
TransportCatalogue::GetBusStat(const std::string_view &bus_name) const {
    auto bus = SearchBus(bus_name);
    if (bus == nullptr) {
        return {};
    }

    std::unordered_set<std::string_view> unique_stops;

    std::pair<double, double> length = std::transform_reduce(
        ++bus->route.begin(), bus->route.end(), bus->route.begin(),
        std::pair<double, double>{0.0, 0.0},
        [](const auto lhs, const auto rhs) {
            return std::pair<double, double>{lhs.first + rhs.first, lhs.second + rhs.second};
        },
        [&](const auto lhs, const auto rhs) {
            unique_stops.insert(lhs->name);
            return std::pair<double, double>{
                GetDistanceBetweenStops(rhs, lhs),
                ComputeDistance(lhs->coordinates, rhs->coordinates)};
        });

    domain::BusStat info;
    info.stops_on_route = bus->route.size();
    info.unique_stops = unique_stops.size();
    info.route_length = length.first;
    info.curvature = length.first / length.second;
    return info;
}

const domain::BusPtrSet *
TransportCatalogue::GetBusesByStop(const std::string_view &stop_name) const {
    if (stop_to_buses_.count(stop_name) > 0) {
        return &stop_to_buses_.at(stop_name);
    }
    return nullptr;
}

bool TransportCatalogue::IsStopInCatalogue(StopPtr stop) const {
    return name_to_stop_.count(stop->name) && name_to_stop_.at(stop->name).get() == stop.get();
}

const domain::BusPtrSet TransportCatalogue::GetBuses() const {
    domain::BusPtrSet buses;
    for (const auto &[_, bus] : name_to_bus_) {
        buses.insert(bus);
    }
    return buses;
}

const domain::StopPtrSet TransportCatalogue::GetStops() const
{
    domain::StopPtrSet stops;
    for (const auto &[_, stop] : name_to_stop_) {
        stops.insert(stop);
    }
    return stops;
}

} // namespace tc