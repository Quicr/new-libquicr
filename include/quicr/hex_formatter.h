#pragma once

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

template<uint8_t Size, uint8_t... N>
class HexFormatter
{
  template <typename... Args>
  struct is_valid_uint : std::bool_constant<(std::is_unsigned_v<Args> && ...)> {};

public:
  HexFormatter() = default;

  template<typename... Args>
  static inline std::string Format(Args... values)
  {
    static_assert(Size == (N + ...), "Total bits cannot exceed specified size");
    static_assert(is_valid_uint<Args...>::value, "Arguments must all be integers");
    static_assert(sizeof...(N) == sizeof...(Args), "Number of values should match distribution of bits");

    std::vector<uint8_t> distribution;
    (distribution.push_back(N), ...);

    std::stringstream ss;
    ss << std::hex << "0x";

    size_t num_args = 0;
    auto get_hex = [&](uint64_t value, size_t i) {
        std::stringstream ss;
        ss << std::hex << std::setw(distribution[i] / 4) << std::setfill('0');
        ss << (value & (distribution[i] >= sizeof(value) * 8 ? ~0x0ull : ~(~0ull << distribution[i])));
        return ss.str();
    };
    (ss << ... << get_hex(values, num_args++));

    return ss.str();
  }
};
