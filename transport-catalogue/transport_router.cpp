#include "transport_router.h"

namespace transport {

Router::Router(int bus_wait_time, double bus_velocity) 
    : bus_wait_time_(bus_wait_time), bus_velocity_(bus_velocity) {
}

void Router::BuildGraph(const Catalogue& catalogue) {
    const auto& all_stops = catalogue.GetSortedAllStops();
    graph_ = graph::DirectedWeightedGraph<double>(all_stops.size() * 2);
    stop_ids_.clear();
    edge_info_.clear();

    // Convert bus velocity from km/h to m/min
    const double velocity_m_per_min = bus_velocity_ * 1000.0 / 60.0;

    // Assign vertex ids to stops
    graph::VertexId vertex_id = 0;
    for (const auto& [stop_name, stop_info] : all_stops) {
        stop_ids_[stop_info->name] = vertex_id;
        
        // Add wait edge (vertex_id -> vertex_id+1 with wait time)
        graph::Edge<double> wait_edge{vertex_id, vertex_id + 1, 
                                    static_cast<double>(bus_wait_time_)};
        auto edge_id = graph_.AddEdge(wait_edge);
        edge_info_[edge_id] = {"", 0, static_cast<double>(bus_wait_time_), stop_info->name};
        
        vertex_id += 2;
    }

    // Process each bus route
    for (const auto& [bus_name, bus_info] : catalogue.GetSortedAllBuses()) {
        const auto& stops = bus_info->stops;
        const size_t stop_count = stops.size();
        
        if (stop_count < 2) continue; // Skip invalid routes

        // Process forward direction
        for (size_t i = 0; i < stop_count - 1; ++i) {
            int distance_sum = 0;
            for (size_t j = i + 1; j < stop_count; ++j) {
                distance_sum += catalogue.GetDistance(stops[j-1], stops[j]);
                double travel_time = distance_sum / velocity_m_per_min;
                
                graph::Edge<double> bus_edge{
                    stop_ids_.at(stops[i]->name) + 1, // from (after waiting)
                    stop_ids_.at(stops[j]->name),     // to (before waiting)
                    travel_time
                };
                auto edge_id = graph_.AddEdge(bus_edge);
                edge_info_[edge_id] = {bus_info->number, static_cast<int>(j - i), 
                                      travel_time, ""};
            }
        }

        // Process backward direction for non-circular routes
        if (!bus_info->is_circle) {
            for (size_t i = stop_count - 1; i > 0; --i) {
                int distance_sum = 0;
                for (size_t j = i - 1; j != static_cast<size_t>(-1); --j) {
                    distance_sum += catalogue.GetDistance(stops[j+1], stops[j]);
                    double travel_time = distance_sum / velocity_m_per_min;
                    
                    graph::Edge<double> reverse_edge{
                        stop_ids_.at(stops[i]->name) + 1, // from (after waiting)
                        stop_ids_.at(stops[j]->name),     // to (before waiting)
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

std::optional<graph::Router<double>::RouteInfo> Router::FindRoute(std::string_view from, std::string_view to) const {
    try {
        if (stop_ids_.count(std::string(from)) == 0 || stop_ids_.count(std::string(to)) == 0) {
            return std::nullopt;
        }
        return router_->BuildRoute(stop_ids_.at(std::string(from)), stop_ids_.at(std::string(to)));
    } catch (...) {
        return std::nullopt;
    }
}

const graph::DirectedWeightedGraph<double>& Router::GetGraph() const {
    return graph_;
}

const std::unordered_map<graph::EdgeId, RouteEdgeInfo>& Router::GetEdgeInfo() const {
    return edge_info_;
}

int Router::GetBusWaitTime() const {
    return bus_wait_time_;
}

double Router::GetBusVelocity() const {
    return bus_velocity_;
}

} // namespace transport
