// json_reader.h
#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"  // Add this include

#include <iostream>
#include <sstream>

namespace json_reader {

struct StopData {
    std::string_view name;
    geo::Coordinates coordinates;
    std::map<std::string_view, int> distances;
};

struct RouteData {
    std::string_view name;
    std::vector<const transport::Stop*> stops;
    bool is_circular;
};

class JsonReader {
public:
    JsonReader(std::istream& input)
        : input_(json::Load(input))
    {}

    const json::Node& GetBaseRequests() const;
    const json::Node& GetStatRequests() const;
    const json::Node& GetRenderSettings() const;
    const json::Node& GetRoutingSettings() const;  // Add this method

    void ProcessRequests(const json::Node& stat_requests, 
                        transport::Catalogue& catalogue, 
                        const renderer::MapRenderer& renderer,
                        transport::Router& router) const;  // Add router parameter

    void FillCatalogue(transport::Catalogue& catalogue);
    renderer::MapRenderer FillRenderSettings(const json::Dict& request_map) const;
    transport::Router FillRoutingSettings() const;  // Add this method

private:
    json::Document input_;
    json::Node dummy_ = nullptr;

    StopData FillStop(const json::Dict& request_map) const;
    void FillStopDistances(transport::Catalogue& catalogue) const;
    RouteData FillRoute(const json::Dict& request_map, transport::Catalogue& catalogue) const;

    std::optional<transport::BusStat> GetBusStat(const transport::Catalogue& catalogue, const std::string_view bus_number) const;
    const std::set<std::string> GetBusesByStop(const transport::Catalogue& catalogue, std::string_view stop_name) const;
    bool IsBusNumber(const transport::Catalogue& catalogue, const std::string_view bus_number) const;
    bool IsStopName(const transport::Catalogue& catalogue, const std::string_view stop_name) const;
    svg::Document RenderMap(const transport::Catalogue& catalogue, const renderer::MapRenderer& renderer) const;

    const json::Node PrintRoute(const json::Dict& request_map, transport::Catalogue& catalogue) const;
    const json::Node PrintStop(const json::Dict& request_map, transport::Catalogue& catalogue) const;
    const json::Node PrintMap(const json::Dict& request_map, const transport::Catalogue& catalogue, const renderer::MapRenderer& renderer) const;
    const json::Node PrintRouting(const json::Dict& request_map, transport::Catalogue& catalogue, transport::Router& router) const;  // Add this method
};

} // namespace json_reader
