#include "json_reader.h"

int main() {
    transport::Catalogue catalogue;
    JsonReader json_doc(std::cin);
    
    // Fill the transport catalogue with data from input
    json_doc.FillCatalogue(catalogue);
    
    // Get render settings and create renderer
    const auto& render_settings = json_doc.GetRenderSettings().AsMap();
    const auto renderer = json_doc.FillRenderSettings(render_settings);
    
    // Process stat requests and output results
    const auto& stat_requests = json_doc.GetStatRequests();
    json_doc.ProcessRequests(stat_requests, catalogue, renderer);
    
    return 0;
}
