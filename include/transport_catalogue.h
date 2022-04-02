#pragma once

#include "domain.h"

#include <deque>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tc {

using Stops = std::unordered_map<std::string_view, domain::StopPtr>;
using Buses = std::unordered_map<std::string_view, domain::BusPtr>;
using StopToBuses = std::unordered_map<std::string_view, domain::BusPtrSet>;
using StopsDist = std::unordered_map<std::pair<domain::StopPtr, domain::StopPtr>,
                                     double,
                                     domain::detail::StopPtrPairHasher>;

class TransportCatalogue {

  private:
    Stops name_to_stop_;
    Buses name_to_bus_;
    StopToBuses stop_to_buses_;
    StopsDist stops_to_distance_;

  public:
    TransportCatalogue(){};

    void AddStop(domain::Stop &stop);
    void AddStop(domain::Stop &&stop);
    void AddStop(domain::StopPtr stop);

    void AddBus(domain::Bus &bus);
    void AddBus(domain::Bus &&bus);
    void AddBus(domain::BusPtr bus);

    domain::StopPtr SearchStop(std::string_view name) const;
    domain::BusPtr SearchBus(std::string_view name) const;

    void
    SetDistanceBetweenStops(domain::StopPtr from, domain::StopPtr to, const double distance);
    void SetDistanceBetweenStops(const std::string_view &from,
                                 const std::string_view &to,
                                 const double distance);

    double GetDistanceBetweenStops(domain::StopPtr from, domain::StopPtr to) const;
    const StopsDist *GetDistances() const;

    const std::optional<domain::BusStat> GetBusStat(const std::string_view &bus_name) const;

    const domain::BusPtrSet *GetBusesByStop(const std::string_view &stop_name) const;

    const domain::BusPtrSet GetBuses() const;
    const domain::StopPtrSet GetStops() const;
    
    const size_t GetStopsCount() const {
        return name_to_stop_.size();
    }

  private:
    bool IsStopInCatalogue(domain::StopPtr stop) const;
};

} // namespace tc