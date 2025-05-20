#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

int main() {
    transport::Catalogue catalogue;
    json_reader::JsonReader json_doc(std::cin);
    
    // 1. Fill the transport catalogue
    json_doc.FillCatalogue(catalogue);
    
    // 2. Configure renderer
    const auto renderer = json_doc.FillRenderSettings(json_doc.GetRenderSettings().AsMap());
    
    // 3. Configure router
    transport::Router router = json_doc.FillRoutingSettings(catalogue);
   
    
    // 4. Process requests
    const auto& stat_requests = json_doc.GetStatRequests();
    json_doc.ProcessRequests(stat_requests, catalogue, renderer, router);
    
    return 0;
}
