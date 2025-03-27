#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility> // for std::pair
#include <regex>

namespace transport {

    namespace utils {

        /**
         * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
         */
        Coordinates ParseCoordinates(std::string_view str) {
            static const double nan = std::nan("");

            auto not_space = str.find_first_not_of(' ');
            auto comma = str.find(',');

            if (comma == str.npos) {
                return { nan, nan };
            }

            auto not_space2 = str.find_first_not_of(' ', comma + 1);

            double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
            double lng = std::stod(std::string(str.substr(not_space2)));

            return { lat, lng };
        }


        std::vector<std::pair<int, std::string>> ParseDistances(const std::string& input) {
            std::vector<std::pair<int, std::string>> result;
            std::regex pattern(R"((\d+)m to ([^,]+))");
            std::sregex_iterator it(input.begin(), input.end(), pattern);
            std::sregex_iterator end;

            for (; it != end; ++it) {
                std::smatch match = *it;
                int distance = std::stoi(match[1].str());
                std::string name = match[2].str();
                result.emplace_back(distance, name);
            }

            return result;
        }
        /**
         * Удаляет пробелы в начале и конце строки
         */
        std::string_view Trim(std::string_view string) {
            const auto start = string.find_first_not_of(' ');
            if (start == string.npos) {
                return {};
            }
            return string.substr(start, string.find_last_not_of(' ') + 1 - start);
        }

        /**
         * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
         */
        std::vector<std::string_view> Split(std::string_view string, char delim) {
            std::vector<std::string_view> result;

            size_t pos = 0;
            while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
                auto delim_pos = string.find(delim, pos);
                if (delim_pos == string.npos) {
                    delim_pos = string.size();
                }
                if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
                    result.push_back(substr);
                }
                pos = delim_pos + 1;
            }

            return result;
        }

    }  // namespace utils

    namespace input {

        /**
         * Парсит маршрут.
         * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
         * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
         */
        std::vector<std::string_view> ParseRoute(std::string_view route) {
            if (route.find('>') != route.npos) {
                return utils::Split(route, '>');
            }

            auto stops = utils::Split(route, '-');
            std::vector<std::string_view> results(stops.begin(), stops.end());
            results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

            return results;
        }

        CommandDescription ParseCommandDescription(std::string_view line) {
            auto colon_pos = line.find(':');
            if (colon_pos == line.npos) {
                return {};
            }

            auto space_pos = line.find(' ');
            if (space_pos >= colon_pos) {
                return {};
            }

            auto not_space = line.find_first_not_of(' ', space_pos);
            if (not_space >= colon_pos) {
                return {};
            }

            return { std::string(line.substr(0, space_pos)),
                    std::string(line.substr(not_space, colon_pos - not_space)),
                    std::string(line.substr(colon_pos + 1)) };
        }

        void InputReader::ParseLine(std::string_view line) {
            auto command_description = ParseCommandDescription(line);
            if (command_description) {
                commands_.push_back(std::move(command_description));
            }
        }

        void InputReader::ApplyCommands(catalogue::TransportCatalogue& catalogue) const {
            // Первый проход — добавляем остановки
            for (const auto& command : commands_) {
                if (command.command == "Stop") {
                    Coordinates coords = utils::ParseCoordinates(command.description);
                    catalogue.AddStop(command.id, coords.lat, coords.lng);
                    std::vector<std::pair<int, std::string>>  distance_betwen_stops = utils::ParseDistances(command.description);
                    catalogue.AddDistance(command.id, distance_betwen_stops);
                }
            }
            // Второй проход - добавляем расстояния между остановками
           /* for (const auto& command : commands_) {
                if (command.command == "Stop") {
                   

                }
            }*/
             
            // Третий проход — добавляем маршруты (зависит от наличия остановок)
            for (const auto& command : commands_) {
                if (command.command == "Bus") {
                    auto stops = ParseRoute(command.description);
                    std::vector<std::string_view> stop_names;
                    stop_names.reserve(stops.size());

                    for (const auto& stop : stops) {
                        stop_names.emplace_back(stop);  // Используем string_view напрямую
                    }

                    bool is_ring_route = command.description.find('>') != std::string::npos;
                    catalogue.AddBus(command.id, stop_names, is_ring_route);
                }
            }
        }

    }  // namespace input

}  // namespace transport
