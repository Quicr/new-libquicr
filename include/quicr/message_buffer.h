#pragma once

#include <quicr/quicr_name.h>
#include <quicr/quicr_namespace.h>

#include <vector>
#include <cassert>

namespace quicr {
enum class uintVar_t : uint64_t {};

namespace messages {
class MessageBuffer
{
public:
  MessageBuffer() = default;
  MessageBuffer(const std::vector<uint8_t>& buffer);
  MessageBuffer(std::vector<uint8_t>&& buffer);

  void push_back(uint8_t t);
  void push_back(const std::vector<uint8_t>& data);
  void pop_back();
  void pop_back(uint16_t len);
  uint8_t back() const { return _buffer.back(); }
  std::vector<uint8_t> back(uint16_t len);
  constexpr bool empty() const { return _buffer.empty(); }

  std::vector<uint8_t>&& move_buffer() { return std::move(_buffer); }

  std::string to_hex() const;
  
  friend void operator<<(MessageBuffer& msg, uint8_t val);
  friend bool operator>>(MessageBuffer& msg, uint8_t& val);

  friend void operator<<(MessageBuffer& msg, const uint64_t& val);
  friend bool operator>>(MessageBuffer& msg, uint64_t& val);

  friend void operator<<(MessageBuffer& msg, const uintVar_t& val);
  friend bool operator>>(MessageBuffer& msg, uintVar_t& val);

  friend void operator<<(MessageBuffer& msg, const std::vector<uint8_t>& val);
  friend bool operator>>(MessageBuffer& msg, std::vector<uint8_t>& val);

private:
  std::vector<uint8_t> _buffer;
};

uintVar_t to_varint(uint64_t);
uint64_t from_varint(uintVar_t);
}
}