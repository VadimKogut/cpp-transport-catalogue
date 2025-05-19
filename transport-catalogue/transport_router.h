#pragma once

#include "router.h"
#include "transport_catalogue.h"
#include "graph.h"

#include <memory>
#include <unordered_map>

namespace transport {

struct RouteEdgeInfo {
    std::string bus_name;
    int span_count;
    double time;
    std::string stop_name;
};

class Router {
public:
    Router() = default;
    Router(int bus_wait_time, double bus_velocity);
    
    void BuildGraph(const Catalogue& catalogue);
    std::optional<graph::Router<double>::RouteInfo> FindRoute(std::string_view from, std::string_view to) const;
    
    const graph::DirectedWeightedGraph<double>& GetGraph() const;
    const std::unordered_map<graph::EdgeId, RouteEdgeInfo>& GetEdgeInfo() const;
    int GetBusWaitTime() const;
    double GetBusVelocity() const;

private:
    int bus_wait_time_ = 0;
    double bus_velocity_ = 0.0;
    
    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::unordered_map<graph::EdgeId, RouteEdgeInfo> edge_info_;
    std::unordered_map<std::string, graph::VertexId> stop_ids_;
};

} // namespace transport
