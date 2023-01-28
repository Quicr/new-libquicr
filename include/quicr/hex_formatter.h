#pragma once

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

template<uint8_t Size, uint8_t... N>
class HexFormatter
{
    template <typename... Args>
    struct is_valid_uint : std::bool_constant<(std::is_unsigned_v<Args> && ...)> {};

    template <typename... Args>
    static inline constexpr bool is_valid_uint_v = is_valid_uint<Args...>::value;

public:
  HexFormatter() = default;

  template<typename... Args>
  HexFormatter(Args... values)
  {
    _output = Format(std::forward<Args>(values)...);
  }

  std::string get() const { return _output; }

  template<typename... Args>
  std::string format(Args... values)
  {
    return Format(std::forward<Args>(values)...);
  }

  template<typename... Args>
  static inline std::string Format(Args... values)
  {
    static_assert(Size == (N + ...), "Total bits cannot exceed specified size");
    static_assert(is_valid_uint_v<Args...>, "Arguments must all be integers");

    std::vector<uint8_t> distribution;
    (distribution.push_back(N), ...);

    std::stringstream ss;
    ss << std::hex;

    size_t num_args = 0;
    auto get_hex = [&](uint64_t value, size_t i) {
        std::stringstream ss;
        ss << std::hex << std::setw(distribution[i] / 4) << std::setfill('0');
        ss << (value & (distribution[i] >= sizeof(value) * 8 ? ~0x0ull : ~(~0ull << distribution[i])));
        return ss.str();
    };
    (ss << ... << get_hex(values, num_args++));

    assert(static_cast<size_t>(num_args) == distribution.size());

    return ss.str();
  }

private:
  std::string _output;
};
