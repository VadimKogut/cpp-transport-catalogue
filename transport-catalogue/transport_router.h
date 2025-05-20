#pragma once 

#include "router.h" 
#include "transport_catalogue.h" 
#include "graph.h" 

#include <memory> 
#include <unordered_map> 
#include <vector>

namespace transport { 

struct RouteEdgeInfo { 
    std::string bus_name; 
    int span_count; 
    double time; 
    std::string stop_name; 
};

struct RouteInfo {
    double total_time;
    std::vector<RouteEdgeInfo> edges;
};

class Router { 
public: 
    Router() = default; 
    Router(const Catalogue& catalogue, int bus_wait_time, double bus_velocity); 
     
    std::optional<RouteInfo> FindRoute(std::string_view from, std::string_view to) const; 
     
private: 
    void BuildGraph(const Catalogue& catalogue);
    
    int bus_wait_time_ = 0; 
    double bus_velocity_ = 0.0; 
     
    graph::DirectedWeightedGraph<double> graph_; 
    std::unique_ptr<graph::Router<double>> router_; 
    std::unordered_map<graph::EdgeId, RouteEdgeInfo> edge_info_; 
    std::unordered_map<std::string, graph::VertexId> stop_ids_; 
}; 

} // namespace transport 
