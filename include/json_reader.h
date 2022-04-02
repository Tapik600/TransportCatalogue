#pragma once

#include "json_builder.h"
#include "request_handler.h"

namespace json::reader {

class JsonReader {

  public:
    JsonReader() = default;

    void ReadRequests(std::istream &input);

    void AddStops(const std::vector<json::Dict> &requests, tc::TransportCatalogue &db) const;
    void AddBuses(const std::vector<json::Dict> &requests, tc::TransportCatalogue &db) const;

    void ExecuteBaseRequest(tc::TransportCatalogue &db) const;
    void ExecuteStatRequest(std::ostream &out, const tc::RequestHandler &handler) const;

    const renderer::RendererSettings GetRendererSettings();
    const std::string GetSerializationSettings();
    const router::RoutingSettings GetRoutingSettings();

  private:
    json::Node GetStopStat(const json::Dict &request, const tc::RequestHandler &handler) const;
    json::Node GetBusStat(const json::Dict &request, const tc::RequestHandler &handler) const;
    json::Node GetMap(const json::Dict &request, const tc::RequestHandler &handler) const;
    json::Node GetRoute(const json::Dict &request, const tc::RequestHandler &handler) const;

    svg::Color ParseColor(const json::Node &node);
    
  private:
    json::Array base_requests_;
    json::Array stat_requests_;
    json::Dict render_settings_;
    json::Dict routing_settings_;
    json::Dict serialization_settings_;
};

} // namespace json::reader