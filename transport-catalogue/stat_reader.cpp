#include "stat_reader.h"
#include "input_reader.h"
#include <iostream>
#include <vector>
#include <string_view>

namespace transport {

    namespace stat {

        void ParseAndPrintStat(const catalogue::TransportCatalogue& transport_catalogue, std::string_view request, std::ostream& output) {
            size_t space_pos = request.find(' ');
            if (space_pos == std::string_view::npos) {
                output << "Invalid request format" << std::endl;
                return;
            }
            if (request.substr(0, space_pos) == "Bus") {
                std::string_view bus_name = request.substr(space_pos + 1); // Берём часть строки после пробела

                const auto* bus = transport_catalogue.GetBus(bus_name);
                if (bus) {
                    output << "Bus " << bus->name << ": " << bus->stops.size() << " stops on route, "
                        << bus->count_unique_stops << " unique stops, " << bus->geo_route << " route length, " <<bus->geo_route/bus->len_route << " curvature" << std::endl;
                }
                else {
                    output << "Bus " << bus_name << ": not found" << std::endl;
                }
            }
            else {
                std::string_view stop_name = request.substr(space_pos + 1); // Берём часть строки после пробела

                const auto* stop = transport_catalogue.GetStop(stop_name);
                if (stop) {
                    if (!stop->buses.empty()) {
                        output << "Stop " << stop->name << ": buses";
                        for (auto& bus : stop->buses) {
                            output << " " << bus;
                        }
                        output << std::endl;
                    }
                    else {
                        output << "Stop " << stop_name << ": no buses" << std::endl;
                    }
                }
                else {
                    output << "Stop " << stop_name << ": not found" << std::endl;
                }
            }
        }

    }  // namespace stat

}  // namespace transport
