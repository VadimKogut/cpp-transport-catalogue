#include "transport_catalogue.h"
#include <cassert>

namespace transport {

    namespace catalogue {

        int TransportCatalogue::CounterUniqStops(const std::vector<std::string_view>& stops) {
            std::unordered_set<std::string_view> unique_stops;

            // Добавляем все элементы в unordered_set
            for (const auto& stop : stops) {
                unique_stops.insert(stop);
            }

            // Возвращаем количество уникальных элементов
            return static_cast<int>(unique_stops.size());
        }

        void TransportCatalogue::AddStop(const std::string& name, double& latitude, double& longitude) {
            stops_.emplace(name, Stop{ name, {} ,Coordinates(latitude, longitude) });
        }

        void TransportCatalogue::AddBus(const std::string& name, std::vector<std::string_view> stops, bool is_ring) {
            int count_uniq_stops = CounterUniqStops(stops);
            double len_route = ComputeLengthRoute(stops, is_ring);  // Вычисляем длину маршрута
            double geo_route = ComputeGeoRoute(stops,is_ring);
            std::deque<std::string> stop_copy;
            for (auto& stop : stops) {
                stop_copy.emplace_back(stop);
            }
            buses_.emplace(name, Bus{ name, stop_copy, count_uniq_stops, len_route,geo_route, is_ring });

            for (auto& stop : stop_copy) {
                if (stops_.count(stop)) {
                    stops_[stop].buses.insert(name);
                }
            }
        }

        
        void TransportCatalogue::AddDistance(const std::string& name, const std::vector<std::pair<int, std::string>>& routes){
            for (auto route : routes) {
                distances_.emplace(std::make_pair(name, route.second), route.first);
            }
        }


        const TransportCatalogue::Bus* TransportCatalogue::GetBus(const std::string_view& bus_name) const {
            auto it = buses_.find(std::string(bus_name));
            if (it != buses_.end()) {
                return &it->second;
            }
            return nullptr;
        }

        const TransportCatalogue::Stop* TransportCatalogue::GetStop(const std::string_view& stop) const {
            auto it = stops_.find(std::string(stop));
            if (it != stops_.end()) {
                return &it->second;
            }
            return nullptr;
        }

        double TransportCatalogue::ComputeDistanceStops(const Stop& stop1, const Stop& stop2) const {
            return ComputeDistance(stop1.position, stop2.position);
        }

        double TransportCatalogue::ComputeLengthRoute(const std::vector<std::string_view>& stops, bool is_ring) const {
            double total_length = 0.0;

            for (size_t i = 0; i + 1 < stops.size(); ++i) {
                auto it1 = stops_.find(std::string(stops[i]));
                auto it2 = stops_.find(std::string(stops[i + 1]));

                if (it1 != stops_.end() && it2 != stops_.end()) {
                    total_length += ComputeDistanceStops(it1->second, it2->second);
                }
            }

            if (is_ring && stops.size() > 1) {
                auto it_start = stops_.find(std::string(stops.front()));
                auto it_end = stops_.find(std::string(stops.back()));

                if (it_start != stops_.end() && it_end != stops_.end()) {
                    total_length += ComputeDistanceStops(it_end->second, it_start->second);
                }
            }

            return total_length;
        }
        
        double TransportCatalogue::ComputeGeoRoute(const std::vector<std::string_view>& stops, bool is_ring) const {
            if (stops.size() < 2) {
                return 0.0;  // Маршрут из 0 или 1 остановки не имеет расстояния
            }

            double total_distance = 0.0;

            // Считаем путь "туда" (от первой до последней остановки)
            for (size_t i = 0; i < stops.size() - 1; ++i) {
                const std::string from(stops[i]);
                const std::string to(stops[i + 1]);

                auto it = distances_.find({ from, to });
                if (it != distances_.end()) {
                    total_distance += it->second;
                }
                else {
                    // Если A→B не найдено, пробуем B→A
                    it = distances_.find({ to, from });
                    if (it != distances_.end()) {
                        total_distance += it->second;
                    }
                    // Если оба варианта отсутствуют, можно считать 0 или бросить исключение
                }
            }

            // Если маршрут НЕ кольцевой, добавляем путь "обратно" (от последней до первой)
            if (is_ring && stops.size() > 1) {
                const std::string from(stops.back());
                const std::string to(stops.front());

                auto it = distances_.find({ from, to });
                if (it != distances_.end()) {
                    total_distance += it->second;
                }
                else {
                    it = distances_.find({ to, from });
                    if (it != distances_.end()) {
                        total_distance += it->second;
                    }
                }
            }

            return total_distance;
        }
    }  // namespace catalogue

}  // namespace transport
