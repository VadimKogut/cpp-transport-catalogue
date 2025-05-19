#include "json_reader.h"
#include "json_builder.h"

using namespace std::literals;

namespace json_reader {

const json::Node& JsonReader::GetBaseRequests() const {
    if (!input_.GetRoot().AsMap().count("base_requests")) return dummy_;
    return input_.GetRoot().AsMap().at("base_requests");
}

const json::Node& JsonReader::GetStatRequests() const {
    if (!input_.GetRoot().AsMap().count("stat_requests")) return dummy_;
    return input_.GetRoot().AsMap().at("stat_requests");
}

const json::Node& JsonReader::GetRenderSettings() const {
    if (!input_.GetRoot().AsMap().count("render_settings")) return dummy_;
    return input_.GetRoot().AsMap().at("render_settings");
}

const json::Node& JsonReader::GetRoutingSettings() const {
    if (!input_.GetRoot().AsMap().count("routing_settings")) return dummy_;
    return input_.GetRoot().AsMap().at("routing_settings");
}

void JsonReader::ProcessRequests(const json::Node& stat_requests, 
                               transport::Catalogue& catalogue,
                               const renderer::MapRenderer& renderer,
                               transport::Router& router) const {
    json::Array result;
    result.reserve(stat_requests.AsArray().size());

    for (const auto& request : stat_requests.AsArray()) {
        const auto& request_map = request.AsMap();
        const auto& type = request_map.at("type").AsString();
        
        if (type == "Stop") {
            result.push_back(PrintStop(request_map, catalogue).AsMap());
        } else if (type == "Bus") {
            result.push_back(PrintRoute(request_map, catalogue).AsMap());
        } else if (type == "Map") {
            result.push_back(PrintMap(request_map, catalogue, renderer).AsMap());
        } else if (type == "Route") {
            result.push_back(PrintRouting(request_map, catalogue, router).AsMap());
        }
    }

    json::Print(json::Document{result}, std::cout);
}

StopData JsonReader::FillStop(const json::Dict& request_map) const {
    StopData data;
    data.name = request_map.at("name").AsString();
    data.coordinates = {
        request_map.at("latitude").AsDouble(),
        request_map.at("longitude").AsDouble()
    };

    const auto& distances = request_map.at("road_distances").AsMap();
    for (const auto& [stop_name, dist] : distances) {
        data.distances.emplace(stop_name, dist.AsInt());
    }
    return data;
}

RouteData JsonReader::FillRoute(const json::Dict& request_map, 
                              transport::Catalogue& catalogue) const {
    RouteData data;
    data.name = request_map.at("name").AsString();
    data.is_circular = request_map.at("is_roundtrip").AsBool();

    const auto& stops_array = request_map.at("stops").AsArray();
    data.stops.reserve(stops_array.size());
    
    for (const auto& stop_node : stops_array) {
        data.stops.push_back(catalogue.FindStop(stop_node.AsString()));
    }

    return data;
}

void JsonReader::FillStopDistances(transport::Catalogue& catalogue) const {
    const auto& arr = GetBaseRequests().AsArray();
    for (const auto& request_stops : arr) {
        const auto& request_map = request_stops.AsMap();
        if (request_map.at("type").AsString() != "Stop") continue;

        const auto stop_data = FillStop(request_map);
        for (const auto& [to_name, dist] : stop_data.distances) {
            auto from = catalogue.FindStop(stop_data.name);
            auto to = catalogue.FindStop(to_name);
            catalogue.SetDistance(from, to, dist);
        }
    }
}

void JsonReader::FillCatalogue(transport::Catalogue& catalogue) {
    const auto& arr = GetBaseRequests().AsArray();
    
    // First pass - add all stops
    for (const auto& request : arr) {
        const auto& request_map = request.AsMap();
        if (request_map.at("type").AsString() != "Stop") continue;

        const auto stop_data = FillStop(request_map);
        catalogue.AddStop(stop_data.name, stop_data.coordinates);
    }

    // Second pass - add distances between stops
    FillStopDistances(catalogue);

    // Third pass - add routes
    for (const auto& request : arr) {
        const auto& request_map = request.AsMap();
        if (request_map.at("type").AsString() != "Bus") continue;

        const auto route_data = FillRoute(request_map, catalogue);
        catalogue.AddRoute(route_data.name, route_data.stops, route_data.is_circular);
    }
}

transport::Router JsonReader::FillRoutingSettings() const {
    const auto& settings_map = GetRoutingSettings().AsMap();
    int bus_wait_time = settings_map.at("bus_wait_time").AsInt();
    double bus_velocity = settings_map.at("bus_velocity").AsDouble();
    return transport::Router(bus_wait_time, bus_velocity);
}

std::optional<transport::BusStat> JsonReader::GetBusStat(
    const transport::Catalogue& catalogue, 
    std::string_view bus_number) const {
    
    const transport::Bus* bus = catalogue.FindRoute(bus_number);
    if (!bus) return std::nullopt;

    transport::BusStat stat;
    stat.stops_count = bus->is_circle ? bus->stops.size() 
                                     : bus->stops.size() * 2 - 1;
    stat.unique_stops_count = catalogue.UniqueStopsCount(bus_number);

    double geographic_length = 0.0;
    stat.route_length = 0;

    for (size_t i = 0; i < bus->stops.size() - 1; ++i) {
        const auto from = bus->stops[i];
        const auto to = bus->stops[i + 1];
        const double direct_distance = geo::ComputeDistance(
            from->coordinates, to->coordinates);

        if (bus->is_circle) {
            stat.route_length += catalogue.GetDistance(from, to);
            geographic_length += direct_distance;
        } else {
            stat.route_length += catalogue.GetDistance(from, to) + 
                               catalogue.GetDistance(to, from);
            geographic_length += direct_distance * 2;
        }
    }

    stat.curvature = stat.route_length / geographic_length;
    return stat;
}

const std::set<std::string> JsonReader::GetBusesByStop(
    const transport::Catalogue& catalogue,
    std::string_view stop_name) const {
    
    const auto* stop = catalogue.FindStop(stop_name);
    return stop ? stop->buses_by_stop : std::set<std::string>{};
}

bool JsonReader::IsBusNumber(const transport::Catalogue& catalogue,
                           std::string_view bus_number) const {
    return catalogue.FindRoute(bus_number) != nullptr;
}

bool JsonReader::IsStopName(const transport::Catalogue& catalogue,
                          std::string_view stop_name) const {
    return catalogue.FindStop(stop_name) != nullptr;
}

svg::Document JsonReader::RenderMap(const transport::Catalogue& catalogue,
                                  const renderer::MapRenderer& renderer) const {
    return renderer.GetSVG(catalogue.GetSortedAllBuses());
}

const json::Node JsonReader::PrintRoute(const json::Dict& request_map,
                                      transport::Catalogue& catalogue) const {
    json::Builder builder;
    builder.StartDict();
    
    const std::string& route_number = request_map.at("name").AsString();
    builder.Key("request_id").Value(request_map.at("id").AsInt());

    if (!IsBusNumber(catalogue, route_number)) {
        builder.Key("error_message").Value("not found"s);
    } else {
        const auto bus_stat = GetBusStat(catalogue, route_number);
        builder.Key("curvature").Value(bus_stat->curvature);
        builder.Key("route_length").Value(bus_stat->route_length);
        builder.Key("stop_count").Value(static_cast<int>(bus_stat->stops_count));
        builder.Key("unique_stop_count").Value(static_cast<int>(bus_stat->unique_stops_count));
    }

    builder.EndDict();
    return builder.Build();
}

const json::Node JsonReader::PrintStop(const json::Dict& request_map,
                                     transport::Catalogue& catalogue) const {
    json::Builder builder;
    builder.StartDict();
    
    const std::string& stop_name = request_map.at("name").AsString();
    builder.Key("request_id").Value(request_map.at("id").AsInt());

    if (!IsStopName(catalogue, stop_name)) {
        builder.Key("error_message").Value("not found"s);
    } else {
        json::Array buses;
        const auto buses_set = GetBusesByStop(catalogue, stop_name);
        buses.reserve(buses_set.size());
        buses.insert(buses.end(), buses_set.begin(), buses_set.end());
        builder.Key("buses").Value(std::move(buses));
    }

    builder.EndDict();
    return builder.Build();
}

const json::Node JsonReader::PrintMap(const json::Dict& request_map,
                                    const transport::Catalogue& catalogue,
                                    const renderer::MapRenderer& renderer) const {
    json::Builder builder;
    builder.StartDict();
    
    builder.Key("request_id").Value(request_map.at("id").AsInt());
    
    std::ostringstream strm;
    RenderMap(catalogue, renderer).Render(strm);
    builder.Key("map").Value(strm.str());

    builder.EndDict();
    return builder.Build();
}

const json::Node JsonReader::PrintRouting(const json::Dict& request_map,
                                        transport::Catalogue& catalogue,
                                        transport::Router& router) const {
    json::Builder builder;
    builder.StartDict();
    
    const std::string& from = request_map.at("from").AsString();
    const std::string& to = request_map.at("to").AsString();
    const int request_id = request_map.at("id").AsInt();
    builder.Key("request_id").Value(request_id);

    const auto* from_stop = catalogue.FindStop(from);
    const auto* to_stop = catalogue.FindStop(to);

    if (!from_stop) {
        builder.Key("error_message").Value("not found"s);
    } else if (!to_stop) {
        builder.Key("error_message").Value("not found"s);
    } else {
        auto route_info = router.FindRoute(from, to);
        if (!route_info) {
            builder.Key("error_message").Value("not found"s);
        } else {
            // Round to 6 decimal places to avoid floating point precision issues
            auto round_time = [](double time) {
                return std::round(time * 1e6) / 1e6;
            };

            builder.Key("total_time").Value(round_time(route_info->weight));
            builder.Key("items").StartArray();
            
            for (const auto& edge_id : route_info->edges) {
                const auto& edge_info = router.GetEdgeInfo().at(edge_id);
                
                if (edge_info.bus_name.empty()) {
                    // Wait activity
                    builder.StartDict()
                        .Key("type").Value("Wait")
                        .Key("stop_name").Value(edge_info.stop_name)
                        .Key("time").Value(round_time(edge_info.time))
                        .EndDict();
                } else {
                    // Bus activity
                    builder.StartDict()
                        .Key("type").Value("Bus")
                        .Key("bus").Value(edge_info.bus_name)
                        .Key("span_count").Value(edge_info.span_count)
                        .Key("time").Value(round_time(edge_info.time))
                        .EndDict();
                }
            }
            builder.EndArray();
        }
    }

    builder.EndDict();
    return builder.Build();
}


renderer::MapRenderer JsonReader::FillRenderSettings(const json::Dict& request_map) const {
    renderer::RenderSettings settings;
    
    settings.width = request_map.at("width").AsDouble();
    settings.height = request_map.at("height").AsDouble();
    settings.padding = request_map.at("padding").AsDouble();
    settings.stop_radius = request_map.at("stop_radius").AsDouble();
    settings.line_width = request_map.at("line_width").AsDouble();
    
    settings.bus_label_font_size = request_map.at("bus_label_font_size").AsInt();
    const auto& bus_offset = request_map.at("bus_label_offset").AsArray();
    settings.bus_label_offset = {bus_offset[0].AsDouble(), bus_offset[1].AsDouble()};
    
    settings.stop_label_font_size = request_map.at("stop_label_font_size").AsInt();
    const auto& stop_offset = request_map.at("stop_label_offset").AsArray();
    settings.stop_label_offset = {stop_offset[0].AsDouble(), stop_offset[1].AsDouble()};
    
    settings.underlayer_width = request_map.at("underlayer_width").AsDouble();
    
    // Parse underlayer color
    const auto& underlayer_color = request_map.at("underlayer_color");
    if (underlayer_color.IsString()) {
        settings.underlayer_color = underlayer_color.AsString();
    } else if (underlayer_color.IsArray()) {
        const auto& color = underlayer_color.AsArray();
        if (color.size() == 3) {
            settings.underlayer_color = svg::Rgb(
                color[0].AsInt(), color[1].AsInt(), color[2].AsInt());
        } else if (color.size() == 4) {
            settings.underlayer_color = svg::Rgba(
                color[0].AsInt(), color[1].AsInt(), color[2].AsInt(), 
                color[3].AsDouble());
        } else {
            throw std::logic_error("Invalid underlayer color format");
        }
    } else {
        throw std::logic_error("Invalid underlayer color type");
    }
    
    // Parse color palette
    const auto& palette = request_map.at("color_palette").AsArray();
    settings.color_palette.reserve(palette.size());
    
    for (const auto& color_node : palette) {
        if (color_node.IsString()) {
            settings.color_palette.push_back(color_node.AsString());
        } else if (color_node.IsArray()) {
            const auto& color = color_node.AsArray();
            if (color.size() == 3) {
                settings.color_palette.emplace_back(
                    svg::Rgb(color[0].AsInt(), color[1].AsInt(), color[2].AsInt()));
            } else if (color.size() == 4) {
                settings.color_palette.emplace_back(
                    svg::Rgba(color[0].AsInt(), color[1].AsInt(), 
                             color[2].AsInt(), color[3].AsDouble()));
            } else {
                throw std::logic_error("Invalid color in palette");
            }
        } else {
            throw std::logic_error("Invalid color type in palette");
        }
    }
    
    return renderer::MapRenderer(settings);
}

} // namespace json_reader
