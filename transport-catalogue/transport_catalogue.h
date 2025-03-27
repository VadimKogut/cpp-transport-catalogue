#pragma once
#ifndef TRANSPORT_CATALOGUE_H
#define TRANSPORT_CATALOGUE_H

#include <deque>
#include <functional> 
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include "geo.h"

namespace transport {

    namespace catalogue {

        class TransportCatalogue {
        public:
            struct Stop {
                std::string name;
                std::set<std::string> buses;  // Маршруты, проходящие через остановку
                Coordinates position;
            };

            struct Bus {
                std::string name;
                std::deque<std::string> stops;  // Все остановки маршрута
                int count_unic_stops = 0;
                double len_route = 0;
                double geo_route = 0;
                bool is_ring = false;  // Флаг кольцевого маршрута
            };
            // Добавление остановки
            void AddStop(const std::string& name, double& latitude, double& longitude);

            // Добавление маршрута
            void AddBus(const std::string& name, std::vector<std::string_view> stops, bool is_ring);

            // Поиск маршрута
            const Bus* GetBus(const std::string_view& bus_name) const;

            // Поиск пересадок
            const Stop* GetStop(const std::string_view& stop) const;

            //Добавление длин маршрутов
            void AddDistance(const std::string& name, const std::vector<std::pair<int, std::string>>& routes);

        private:
            struct StringPairHash {
                size_t operator()(const std::pair<std::string, std::string>& p) const {
                    return std::hash<std::string>{}(p.first) ^ std::hash<std::string>{}(p.second);
                }
            };
            // Получение расстояния между остановками
            double ComputeDistanceStops(const Stop& stop1, const Stop& stop2) const;
            //Получения полного пути
            double ComputeGeoRoute(const std::vector<std::string_view>& stops, bool is_ring) const;
            // Считаем количество уникальных остановок
            int CounterUniqStops(const std::vector<std::string_view>& stops);

            // Считаем общую длину пути
            double ComputeLengthRoute(const std::vector<std::string_view>& stops, bool is_ring) const;

            std::unordered_map<std::string, Stop> stops_;  // Ключ: название остановки
            std::unordered_map<std::string, Bus> buses_;   // Ключ: название маршрута
        
            std::unordered_map<std::pair<std::string, std::string>,int, StringPairHash> distances_;
        };

    }  // namespace catalogue

}  // namespace transport

#endif // TRANSPORT_CATALOGUE_H
