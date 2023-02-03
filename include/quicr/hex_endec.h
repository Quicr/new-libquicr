#pragma once

#include <array>
#include <bitset>
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

    std::vector<uint8_t> distribution;
    (distribution.push_back(N), ...);

    return Encode(distribution, std::forward<UInt_ts>(values)...);
  }
  
  template<typename... UInt_ts>
  static inline std::string Encode(const std::vector<uint8_t>& distribution, UInt_ts... values)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(is_valid_uint<UInt_ts...>::value, "Arguments must all be unsigned integers");
    assert(distribution.size() == sizeof...(UInt_ts) && "Number of values should match distribution of bits");

    std::vector<uint64_t> vals;
    (vals.push_back(values), ...);

    return Encode(distribution, vals);
  }

  static inline std::string Encode(const std::vector<uint8_t>& distribution, const std::vector<uint64_t>& values)
  {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    assert(distribution.size() == values.size() && "Number of values should match distribution of bits");

    std::stringstream ss;
    ss << std::hex << "0x";

    std::bitset<Size> bits;
    uint8_t remaining_bits = Size;
    int i = 0;
    for (const auto& value : values)
    {
      uint8_t dist = distribution.at(i++);
      remaining_bits -= dist;
      bits |= std::bitset<Size>(value) << remaining_bits;
    };

    if (Size > sizeof(uint64_t) * 8)
    {
      static const std::bitset<Size> bitset_divider(std::numeric_limits<uint64_t>::max());
      ss << std::setw(Size/8) << std::setfill('0') << (bits >> (Size / 2) & bitset_divider).to_ullong();
      ss << std::setw(Size/8) << std::setfill('0') << (bits & bitset_divider).to_ullong();
    }
    else
    {
      ss << std::setw(Size/4) << std::setfill('0') << bits.to_ullong();
    }

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
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    static_assert(is_valid_uint<Uint_t>::value, "Type must be unsigned integer");

    std::string clean_hex = hex;
    auto found = clean_hex.find("0x");
    if (found != std::string::npos)
      clean_hex.erase(found, 2);

    if (clean_hex.length() != Size / 4)
      throw std::runtime_error("Hex string value must be " +
                               std::to_string(Size / 4) + " characters (" +
                               std::to_string(Size / 8) + " bytes)");

    std::bitset<Size> bits;
    const uint8_t size_of = sizeof(uint64_t) * 2;
    if (Size > size_of * 4)
    {
      size_t midpoint = clean_hex.length() - size_of;    
      std::string low_bits = "0x" + clean_hex.substr(midpoint, size_of);
      clean_hex.erase(midpoint, size_of);
      std::string hi_bits = "0x" + clean_hex;

      auto hi = std::stoull(hi_bits, nullptr, 16);
      auto low = std::stoull(low_bits, nullptr, 16);

      bits |= std::bitset<Size>(low) | (std::bitset<Size>(hi) << (Size / 2));
    }
    else
    {
      bits |= std::bitset<Size>(std::stoull(clean_hex, nullptr, 16));
    }

    std::vector<Uint_t> result(distribution.size());
    for (int i = distribution.size() - 1; i >= 0; --i)
    {
      std::bitset<Size> mask = std::bitset<Size>{}.set();
      
      auto dist = distribution.at(i);
      mask <<= dist;
      mask.flip();

      result[i] = (bits & mask).to_ullong();
      bits >>= dist;
    };

    return result;
  }
};
}
