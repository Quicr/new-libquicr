#pragma once

#include <quicr/quicr_name.h>

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <limits>
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
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(Size == (N + ...), "Total bits must be equal to Size");
  }

  /**
   * @brief Encodes the last N bits of values in order according to distribution
   *        of N and builds a hex string that is the size in bits of Size.
   * 
   * @tparam ...UInt_ts The unsigned integer types to be passed.
   * @param ...values The unsigned values to be encoded into the hex string.
   *
   * @returns Hex string containing the provided values distributed according to
   *          N in order.
   */
  template<typename... UInt_ts>
  static inline std::string Encode(UInt_ts... values)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(Size == (N + ...), "Total bits cannot exceed specified size");
    static_assert(sizeof...(N) == sizeof...(UInt_ts),
                  "Number of values should match distribution of bits");

    std::vector<uint8_t> distribution{N...};
    return Encode(distribution, std::forward<UInt_ts>(values)...);
  }
  
  template<typename... UInt_ts>
  static inline std::string Encode(const std::vector<uint8_t>& distribution, UInt_ts... values)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(is_valid_uint<UInt_ts...>::value, "Arguments must all be unsigned integers");
    assert(distribution.size() == sizeof...(UInt_ts) && "Number of values should match distribution of bits");

    std::vector<uint64_t> vals{values...};
    return Encode(distribution, vals);
  }

  template<bool B = Size <= sizeof(uint64_t) * 8>
  static inline typename std::enable_if<B, std::string>::type
  Encode(const std::vector<uint8_t>& distribution, const std::vector<uint64_t>& values)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    assert(distribution.size() == values.size() && "Number of values should match distribution of bits");

    std::ostringstream ss;
    ss << std::hex << "0x" << std::setfill('0');
    
    uint8_t current_distribution = 0;
    uint64_t bits = 0;
    for (const auto& value : values)
    {
      const uint8_t dist = distribution.at(current_distribution++);
      bits <<= dist;
      bits |= (value & ~(~0x0ull << dist));
    };
    
    ss << std::setw(Size / 4) << bits;

    return ss.str();
  }

  template<bool B = Size <= sizeof(uint64_t) * 8>
  static inline typename std::enable_if<!B, std::string>::type
  Encode(const std::vector<uint8_t>& distribution, const std::vector<uint64_t>& values)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    assert(distribution.size() == values.size() && "Number of values should match distribution of bits");

    std::ostringstream ss;
    ss << std::hex << "0x" << std::setfill('0');
    
    uint8_t current_distribution = 0;
    std::bitset<Size> bits;
    for (const auto& value : values)
    {
      uint8_t dist = distribution.at(current_distribution++);
      bits <<= dist;
      bits |= std::bitset<Size>(dist >= sizeof(uint64_t) * 8 ? value : (value & ~(~0x0ull << dist)));
    };

    static const std::bitset<Size> bitset_divider(std::numeric_limits<uint64_t>::max());
    ss << std::setw(Size / 8) << (bits >> (Size / 2) & bitset_divider).to_ullong();
    ss << std::setw(Size / 8) << (bits & bitset_divider).to_ullong();
    
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
   * @returns Structured binding of values decoded from hex string corresponding
   *          in order to the size of N.
   */
  template<typename Uint_t = uint64_t>
  static inline std::array<Uint_t, sizeof...(N)> Decode(const std::string& hex)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(Size >= (N + ...), "Total bits cannot exceed specified size");
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");

    std::vector<uint8_t> distribution{N...};
    auto result = Decode(distribution, hex);
    std::array<Uint_t, sizeof...(N)> out;
    std::copy_n(std::make_move_iterator(result.begin()), sizeof...(N), out.begin());

    return out;
  }

  template<typename Uint_t = uint64_t>
  static inline std::array<Uint_t, sizeof...(N)> Decode(const quicr::Name& name)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(Size >= (N + ...), "Total bits cannot exceed specified size");
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");

    std::vector<uint8_t> distribution{N...};
    auto result = Decode(distribution, name.to_hex());
    std::array<Uint_t, sizeof...(N)> out;
    std::copy_n(std::make_move_iterator(result.begin()), sizeof...(N), out.begin());

    return out;
  }
  
  template<typename Uint_t = uint64_t, bool B = Size <= sizeof(uint64_t) * 8>
  static inline typename std::enable_if<B, std::vector<Uint_t>>::type
  Decode(const std::vector<uint8_t>& distribution, const std::string& hex)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");

    const auto dist_size = distribution.size();
    std::vector<uint64_t> result(dist_size);
    uint64_t bits = std::stoull(hex, nullptr, 16);
    for (int i = dist_size - 1; i >= 0; --i)
    {
      const auto dist = distribution.at(i);
      result[i] = bits & ~(~0x0ull << dist);
      bits >>= dist;
    }

    return result;
  }

  template<typename Uint_t = uint64_t, bool B = Size <= sizeof(uint64_t) * 8>
  static inline typename std::enable_if<!B, std::vector<Uint_t>>::type
  Decode(const std::vector<uint8_t>& distribution, const std::string& hex)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");


    std::string clean_hex = hex;
    if (clean_hex.substr(0, 2) == "0x")
      clean_hex.erase(0, 2);

    constexpr uint8_t hex_length = Size / 4;
    if (clean_hex.length() != hex_length)
      throw std::runtime_error("Hex string value must be " +
                              std::to_string(hex_length) + " characters (" +
                              std::to_string(Size / 8) + " bytes)");

    std::bitset<Size> bits;
    const uint8_t section_length = hex_length / 2;
    constexpr size_t sizeof_uint64_bits = sizeof(uint64_t) * 8;
    constexpr size_t num_sections = Size / sizeof_uint64_bits;
    for (size_t i = 0, j = 0; i < (section_length * num_sections); i += section_length)
    {
      const size_t midpoint = i + section_length;
      std::string section = clean_hex.substr(i, midpoint);
      bits |= std::bitset<Size>(std::stoull(section, nullptr, 16)) << (sizeof_uint64_bits * (num_sections - ++j));
    }

    const auto dist_size = distribution.size();
    std::vector<uint64_t> result(dist_size);
    for (size_t i = 0; i < dist_size; ++i)
    {
      const auto dist = distribution.at(i);
      result[i] = (bits >> (Size - dist)).to_ullong();
      bits <<= dist;
    }

    return result;
  }
  
  template<typename Uint_t = uint64_t>
  static inline std::vector<Uint_t> Decode(const std::vector<uint8_t>& distribution, const quicr::Name& name)
  {
    return Decode(distribution, name.to_hex());
  }
};
}
