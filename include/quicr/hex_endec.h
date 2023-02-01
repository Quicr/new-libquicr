#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>

namespace quicr {
/**
 * @brief Encodes/Decodes a hex string from/into a list of unsigned integers
 *        values.
 *
 * The hex string created by/passed to this class is of the format:
 *     0xXX...XYY...YZZ...Z....
 *       |____||____||____|
 *         N0    N1    N2   ...
 *       |_____________________|
 *                Size
 * Where N is the distribution of bits for each value provided. For Example:
 *   HexEndec<64, 32, 24, 8>
 * Describes a 64 bit hex value, distributed into 3 sections 32bit, 24bit, and
 * 8bit respectively.
 *
 * @tparam Size The maximum size in bits of the Hex string
 * @tparam ...N The distribution of bits for each value passed.
 */
template<uint8_t Size, uint8_t... N>
class HexEndec
{
  template<typename... UInt_ts>
  struct is_valid_uint
    : std::bool_constant<(std::is_unsigned_v<UInt_ts> && ...)>
  {
  };

public:
  HexEndec()
  {
    static_assert(Size == (N + ...), "Total bits cannot exceed specified size");
  }

  /**
   * @brief Encodes the last N bits of values in order according to distribution
   *        of N and builds a hex string that is the size in bits of Size.
   * 
   * @tparam ...UInt_ts The unsigned integer types to be passed.
   * @param ...values The unsigned values to be encoded into the hex string.
   *
   * @returns Hex string containing the provided values distributed according to
   * N in order.
   */
  template<typename... UInt_ts>
  static inline std::string Encode(UInt_ts... values)
  {
    static_assert(Size == (N + ...), "Total bits cannot exceed specified size");
    static_assert(sizeof...(N) == sizeof...(UInt_ts),
                  "Number of values should match distribution of bits");

    std::vector<uint8_t> distribution;
    (distribution.push_back(N), ...);

    return Encode(distribution, std::forward<UInt_ts>(values)...);
  }
  
  template<typename... UInt_ts>
  static inline std::string Encode(const std::vector<uint8_t>& distribution, UInt_ts... values)
  {
    static_assert(is_valid_uint<UInt_ts...>::value,
                  "Arguments must all be unsigned integers");

    std::stringstream ss;
    ss << std::hex << "0x";

    size_t num_args = 0;
    auto get_hex = [&](uint64_t value, size_t i) {
      std::stringstream ss;
      ss << std::hex << std::setw(distribution.at(i) / 4) << std::setfill('0');
      ss << (value & (distribution.at(i) >= sizeof(value) * 8
                        ? ~0x0ull
                        : ~(~0ull << distribution.at(i))));
      return ss.str();
    };
    (ss << ... << get_hex(values, num_args++));

    return ss.str();
  }

  /**
   * @brief Decodes a hex string that has size in bits of Size into a list of
   *        values sized according to N in order.
   * 
   * @tparam Uint_t The unsigned integer type to return.
   * @param hex The hex string to decode. Must have a length in bytes
   *            corresponding to the size in bits of Size.
   *
   * @returns Structred binding of values decoded from hex string corresponding
   *          in order to the size of N.
   */
  template<typename Uint_t = uint64_t>
  static inline std::array<Uint_t, sizeof...(N)> Decode(const std::string& hex)
  {
    static_assert(Size == (N + ...), "Total bits cannot exceed specified size");
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");

    std::vector<uint8_t> distribution;
    (distribution.push_back(N), ...);

    auto result = Decode(distribution, hex);
    std::array<Uint_t, sizeof...(N)> out;
    std::copy_n(result.begin(), sizeof...(N), out.begin());

    return out;
  }
  
  template<typename Uint_t = uint64_t>
  static inline std::vector<Uint_t> Decode(const std::vector<uint8_t>& distribution, const std::string& hex)
  {
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");

    std::string clean_hex = hex;
    auto found = clean_hex.find("0x");
    if (found != std::string::npos)
      clean_hex.erase(found, 2);

    if (clean_hex.length() != Size / 4)
      throw std::runtime_error("Hex string value must be " +
                               std::to_string(Size / 4) + " characters (" +
                               std::to_string(Size / 8) + " bytes)");

    int i = 0;
    std::vector<Uint_t> result(distribution.size());
    for (const uint8_t& n : distribution) {
      size_t midpoint = (n / 4);
      std::string bytes = clean_hex.substr(0, midpoint);
      clean_hex.erase(0, midpoint);
      result[i++] = std::stoull(bytes, nullptr, 16);
    }

    return result;
  }
};
}
