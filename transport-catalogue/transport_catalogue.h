#pragma once
#ifndef TRANSPORT_CATALOGUE_H
#define TRANSPORT_CATALOGUE_H

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <string_view>
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

private:
    // Получение расстояния между остановками
    double ComputeDistanceStops(const Stop& stop1, const Stop& stop2) const;

    // Считаем количество уникальных остановок
    int CounterUniqStops(const std::vector<std::string_view>& stops);

    // Считаем общую длину пути
    double ComputeLengthRoute(const std::vector<std::string_view>& stops, bool is_ring) const;

    std::unordered_map<std::string, Stop> stops_;  // Ключ: название остановки
    std::unordered_map<std::string, Bus> buses_;   // Ключ: название маршрута
};

}  // namespace catalogue

}  // namespace transport

#endif // TRANSPORT_CATALOGUE_H
