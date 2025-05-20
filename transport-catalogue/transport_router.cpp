#include "transport_router.h"

namespace transport {

namespace {
constexpr double KM_TO_M = 1000.0;
constexpr double HOUR_TO_MIN = 60.0;
} // namespace

Router::Router(const Catalogue& catalogue, int bus_wait_time, double bus_velocity) 
    : bus_wait_time_(bus_wait_time), bus_velocity_(bus_velocity) {
    BuildGraph(catalogue);
}

void Router::BuildGraph(const Catalogue& catalogue) {
    const auto& all_stops = catalogue.GetSortedAllStops();
    graph_ = graph::DirectedWeightedGraph<double>(all_stops.size() * 2);
    stop_ids_.clear();
    edge_info_.clear();

    const double velocity_m_per_min = bus_velocity_ * KM_TO_M / HOUR_TO_MIN;

    graph::VertexId vertex_id = 0;
    for (const auto& [stop_name, stop_info] : all_stops) {
        stop_ids_[stop_info->name] = vertex_id;
        
        graph::Edge<double> wait_edge{vertex_id, vertex_id + 1, 
                                    static_cast<double>(bus_wait_time_)};
        auto edge_id = graph_.AddEdge(wait_edge);
        edge_info_[edge_id] = {"", 0, static_cast<double>(bus_wait_time_), stop_info->name};
        
        vertex_id += 2;
    }

    for (const auto& [bus_name, bus_info] : catalogue.GetSortedAllBuses()) {
        const auto& stops = bus_info->stops;
        const size_t stop_count = stops.size();
        
        if (stop_count < 2) continue;

        for (size_t i = 0; i < stop_count - 1; ++i) {
            int distance_sum = 0;
            for (size_t j = i + 1; j < stop_count; ++j) {
                distance_sum += catalogue.GetDistance(stops[j-1], stops[j]);
                double travel_time = distance_sum / velocity_m_per_min;
                
                graph::Edge<double> bus_edge{
                    stop_ids_.at(stops[i]->name) + 1,
                    stop_ids_.at(stops[j]->name),
                    travel_time
                };
                auto edge_id = graph_.AddEdge(bus_edge);
                edge_info_[edge_id] = {bus_info->number, static_cast<int>(j - i), 
                                      travel_time, ""};
            }
        }

        if (!bus_info->is_circle) {
            for (size_t i = stop_count - 1; i > 0; --i) {
                int distance_sum = 0;
                for (size_t j = i - 1; j != static_cast<size_t>(-1); --j) {
                    distance_sum += catalogue.GetDistance(stops[j+1], stops[j]);
                    double travel_time = distance_sum / velocity_m_per_min;
                    
                    graph::Edge<double> reverse_edge{
                        stop_ids_.at(stops[i]->name) + 1,
                        stop_ids_.at(stops[j]->name),
                        travel_time
                    };
                    auto edge_id = graph_.AddEdge(reverse_edge);
                    edge_info_[edge_id] = {bus_info->number, static_cast<int>(i - j), 
                                          travel_time, ""};
                }
            }
        }
    }

    router_ = std::make_unique<graph::Router<double>>(graph_);
}

std::optional<RouteInfo> Router::FindRoute(std::string_view from, std::string_view to) const {
    try {
        if (stop_ids_.count(std::string(from)) == 0 || stop_ids_.count(std::string(to)) == 0) {
            return std::nullopt;
        }
        
        auto route = router_->BuildRoute(stop_ids_.at(std::string(from)), stop_ids_.at(std::string(to)));
        if (!route) {
            return std::nullopt;
        }
        
        RouteInfo result;
        result.total_time = route->weight;
        
        for (const auto& edge_id : route->edges) {
            result.edges.push_back(edge_info_.at(edge_id));
        }
        
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace transport
