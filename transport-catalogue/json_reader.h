// json_reader.h
#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

#include <iostream>
#include <sstream>
class JsonReader {
public:
    JsonReader(std::istream& input)
        : input_(json::Load(input))
    {}

    const json::Node& GetBaseRequests() const;
    const json::Node& GetStatRequests() const;
    const json::Node& GetRenderSettings() const;

    void ProcessRequests(const json::Node& stat_requests, transport::Catalogue& catalogue, const renderer::MapRenderer& renderer) const;

    void FillCatalogue(transport::Catalogue& catalogue);
    renderer::MapRenderer FillRenderSettings(const json::Dict& request_map) const;

private:
    json::Document input_;
    json::Node dummy_ = nullptr;

    std::tuple<std::string_view, geo::Coordinates, std::map<std::string_view, int>> FillStop(const json::Dict& request_map) const;
    void FillStopDistances(transport::Catalogue& catalogue) const;
    std::tuple<std::string_view, std::vector<const transport::Stop*>, bool> FillRoute(const json::Dict& request_map, transport::Catalogue& catalogue) const;

    std::optional<transport::BusStat> GetBusStat(const transport::Catalogue& catalogue, const std::string_view bus_number) const;
    const std::set<std::string> GetBusesByStop(const transport::Catalogue& catalogue, std::string_view stop_name) const;
    bool IsBusNumber(const transport::Catalogue& catalogue, const std::string_view bus_number) const;
    bool IsStopName(const transport::Catalogue& catalogue, const std::string_view stop_name) const;
    svg::Document RenderMap(const transport::Catalogue& catalogue, const renderer::MapRenderer& renderer) const;

    const json::Node PrintRoute(const json::Dict& request_map, transport::Catalogue& catalogue) const;
    const json::Node PrintStop(const json::Dict& request_map, transport::Catalogue& catalogue) const;
    const json::Node PrintMap(const json::Dict& request_map, const transport::Catalogue& catalogue, const renderer::MapRenderer& renderer) const;
};
