#pragma once

#include <quicr/quicr_name.h>
#include <quicr/quicr_namespace.h>

#include <cassert>
#include <deque>
#include <vector>

namespace quicr {
/**
 * @brief Variable length integer
 */
enum class uintVar_t : uint64_t
{
};

uintVar_t to_varint(uint64_t);
uint64_t from_varint(uintVar_t);

namespace messages {
/**
 * @brief Defines a buffer that can be sent over transport. Cannot be copied.
 */
class MessageBuffer
{
public:
  MessageBuffer() = default;
  MessageBuffer(const MessageBuffer& other) = delete;
  MessageBuffer(MessageBuffer&& other);
  MessageBuffer(const std::vector<uint8_t>& buffer);
  MessageBuffer(std::vector<uint8_t>&& buffer);
  ~MessageBuffer() = default;

  bool empty() const { return _buffer.empty(); }

  void push(uint8_t t) { _buffer.push_back(t); }
  void pop();
  uint8_t front() const { return _buffer.front(); }

  void push(const std::vector<uint8_t>& data);
  void push(std::vector<uint8_t>&& data);
  void pop(uint16_t len);
  std::vector<uint8_t> front(uint16_t len);

  std::vector<uint8_t> get();

  std::string to_hex() const;

  void operator=(const MessageBuffer& other) = delete;
  void operator=(MessageBuffer&& other);

private:
  std::vector<uint8_t> _buffer;
};

struct MessageBufferException : public std::runtime_error
{
  using std::runtime_error::runtime_error;
};

MessageBuffer&
operator<<(MessageBuffer& msg, uint8_t val);
MessageBuffer&
operator>>(MessageBuffer& msg, uint8_t& val);

MessageBuffer&
operator<<(MessageBuffer& msg, const uint64_t& val);
MessageBuffer&
operator>>(MessageBuffer& msg, uint64_t& val);

MessageBuffer&
operator<<(MessageBuffer& msg, const uintVar_t& val);
MessageBuffer&
operator>>(MessageBuffer& msg, uintVar_t& val);

MessageBuffer&
operator<<(MessageBuffer& msg, const std::vector<uint8_t>& val);
MessageBuffer&
operator>>(MessageBuffer& msg, std::vector<uint8_t>& val);
}
}
