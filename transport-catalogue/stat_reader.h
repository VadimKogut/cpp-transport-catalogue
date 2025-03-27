#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

namespace transport {

    namespace stat {

        void ParseAndPrintStat(const catalogue::TransportCatalogue& transport_catalogue, std::string_view request,
            std::ostream& output);

    }  // namespace stat

}  // namespace transport
