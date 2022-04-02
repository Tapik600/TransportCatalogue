#pragma once

#include "geo.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace domain {

struct Stop {
    std::string name;
    geo::Coordinates coordinates;
};
using StopPtr = std::shared_ptr<Stop>;

struct Bus {
    std::string name;
    std::vector<StopPtr> route;
    bool is_roundtrip = false;
    StopPtr final_stop;
};
using BusPtr = std::shared_ptr<Bus>;

struct BusStat {
    size_t stops_on_route = 0;
    size_t unique_stops = 0;
    double route_length = 0.0;
    double curvature = 0.0;
};

namespace detail {

struct StopPtrPairHasher {
    size_t operator()(const std::pair<StopPtr, StopPtr> &stops) const {
        std::hash<void *> hasher;
        size_t h_from = hasher(stops.first.get());
        size_t h_to = hasher(stops.second.get());
        return h_from * 31 + h_to;
    }
};

struct StopPtrComparator {
    bool operator()(const StopPtr lhs, const StopPtr rhs) const {
        return std::lexicographical_compare(lhs->name.begin(), lhs->name.end(),
                                            rhs->name.begin(), rhs->name.end());
    }
};

struct BusPtrComparator {
    bool operator()(const BusPtr lhs, const BusPtr rhs) const {
        return std::lexicographical_compare(lhs->name.begin(), lhs->name.end(),
                                            rhs->name.begin(), rhs->name.end());
    }
};
} // namespace detail

using StopPtrSet = std::set<StopPtr, detail::StopPtrComparator>;
using BusPtrSet = std::set<BusPtr, detail::BusPtrComparator>;

} // namespace domain
