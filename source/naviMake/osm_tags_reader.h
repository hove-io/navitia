#include <bitset>
#include <map>

namespace navitia { namespace georef {

constexpr uint8_t CYCLE_FWD = 0;
constexpr uint8_t CYCLE_BWD = 1;
constexpr uint8_t CAR_FWD = 2;
constexpr uint8_t CAR_BWD = 3;
constexpr uint8_t FOOT_FWD = 4;
constexpr uint8_t FOOT_BWD = 5;

/// Retourne les propriétés quant aux modes et sens qui peuvent utiliser le way
std::bitset<8> parse_way_tags(const std::map<std::string, std::string> & tags);

}}
