#include <quicr/message_buffer.h>

#include <iomanip>
#include <sstream>
#include <cassert>

namespace quicr {
uintVar_t
to_varint(uint64_t v)
{
  assert(v < 0x1ull << 61);
  return static_cast<uintVar_t>(v);
}

uint64_t
from_varint(uintVar_t v)
{
  return static_cast<uint64_t>(v);
}

namespace messages {
MessageBuffer::MessageBuffer(MessageBuffer&& other)
  : _buffer{ std::move(other._buffer) }
{
}

MessageBuffer::MessageBuffer(const std::vector<uint8_t>& buffer)
  : _buffer{ buffer }
{
}

MessageBuffer::MessageBuffer(std::vector<uint8_t>&& buffer)
  : _buffer{ std::move(buffer) }
{
}

void
MessageBuffer::pop()
{
  assert(!_buffer.empty());
  _buffer.erase(_buffer.begin());
}

void
MessageBuffer::push(const std::vector<uint8_t>& data)
{
  std::copy(data.begin(), data.end(), std::back_inserter(_buffer));
}

void
MessageBuffer::push(std::vector<uint8_t>&& data)
{
  _buffer.insert(_buffer.end(),
                 std::make_move_iterator(data.begin()),
                 std::make_move_iterator(data.end()));
}

void
MessageBuffer::pop(uint16_t len)
{
  if (len > _buffer.size())
    throw std::out_of_range("len cannot be longer than the size of the buffer");

  _buffer.erase(_buffer.begin(), std::next(_buffer.begin(), len));
};

std::vector<uint8_t>
MessageBuffer::front(uint16_t len)
{
  if (len > _buffer.size())
    throw std::out_of_range("len cannot be longer than the size of the buffer");

  return { _buffer.begin(), std::next(_buffer.begin(), len) };
}

std::vector<uint8_t>
MessageBuffer::get()
{
  return std::move(_buffer);
}

std::string
MessageBuffer::to_hex() const
{
  std::ostringstream hex;
  hex << std::hex << std::setfill('0');
  for (const auto& byte : _buffer) {
    hex << std::setw(2) << int(byte);
  }
  return hex.str();
}

void
MessageBuffer::operator=(MessageBuffer&& other)
{
  _buffer = std::move(other._buffer);
}

MessageBuffer&
operator<<(MessageBuffer& msg, const uint8_t val)
{
  msg.push(val);
  return msg;
}

MessageBuffer&
operator>>(MessageBuffer& msg, uint8_t& val)
{
  if (msg.empty()) {
    throw MessageBufferException("Cannot read from empty message buffer");
  }

  val = msg.front();
  msg.pop();
  return msg;
}

MessageBuffer&
operator<<(MessageBuffer& msg, const uint64_t& val)
{
  // TODO - std::copy version for little endian machines optimization

  // buffer on wire is little endian (that is *not* network byte order)
  msg.push(uint8_t((val >> 56) & 0xFF));
  msg.push(uint8_t((val >> 48) & 0xFF));
  msg.push(uint8_t((val >> 40) & 0xFF));
  msg.push(uint8_t((val >> 32) & 0xFF));
  msg.push(uint8_t((val >> 24) & 0xFF));
  msg.push(uint8_t((val >> 16) & 0xFF));
  msg.push(uint8_t((val >> 8) & 0xFF));
  msg.push(uint8_t((val >> 0) & 0xFF));

  return msg;
}

MessageBuffer&
operator>>(MessageBuffer& msg, uint64_t& val)
{
  uint8_t byte[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  msg >> byte[7];
  msg >> byte[6];
  msg >> byte[5];
  msg >> byte[4];
  msg >> byte[3];
  msg >> byte[2];
  msg >> byte[1];
  msg >> byte[0];

  val = (uint64_t(byte[0]) << 0) + (uint64_t(byte[1]) << 8) +
        (uint64_t(byte[2]) << 16) + (uint64_t(byte[3]) << 24) +
        (uint64_t(byte[4]) << 32) + (uint64_t(byte[5]) << 40) +
        (uint64_t(byte[6]) << 48) + (uint64_t(byte[7]) << 56);

  return msg;
}

MessageBuffer&
operator<<(MessageBuffer& msg, const std::vector<uint8_t>& val)
{
  msg << to_varint(val.size());
  msg.push(val);
  return msg;
}

MessageBuffer&
operator>>(MessageBuffer& msg, std::vector<uint8_t>& val)
{
  uintVar_t vecSize{ 0 };
  msg >> vecSize;

  size_t len = from_varint(vecSize);
  if (len == 0) {
    throw MessageBufferException("Decoded vector size is 0");
  }

  val = msg.front(len);
  msg.pop(len);

  return msg;
}

MessageBuffer&
operator<<(MessageBuffer& msg, const uintVar_t& v)
{
  uint64_t val = from_varint(v);

  if (val >= ((uint64_t)1 << 61)) {
    throw MessageBufferException(
      "uintVar_t cannot have a value greater than 1 << 61");
  }

  if (val < ((uint64_t)1 << 7)) {
    msg.push(uint8_t(((val >> 0) & 0x7F)) | 0x00);
    return msg;
  }

  if (val < ((uint64_t)1 << 14)) {
    msg.push(uint8_t(((val >> 8) & 0x3F) | 0x80));
    msg.push(uint8_t((val >> 0) & 0xFF));
    return msg;
  }

  if (val < ((uint64_t)1 << 29)) {
    msg.push(uint8_t(((val >> 24) & 0x1F) | 0x80 | 0x40));
    msg.push(uint8_t((val >> 16) & 0xFF));
    msg.push(uint8_t((val >> 8) & 0xFF));
    msg.push(uint8_t((val >> 0) & 0xFF));
    return msg;
  }

  msg.push(uint8_t(((val >> 56) & 0x0F) | 0x80 | 0x40 | 0x20));
  msg.push(uint8_t((val >> 48) & 0xFF));
  msg.push(uint8_t((val >> 40) & 0xFF));
  msg.push(uint8_t((val >> 32) & 0xFF));
  msg.push(uint8_t((val >> 24) & 0xFF));
  msg.push(uint8_t((val >> 16) & 0xFF));
  msg.push(uint8_t((val >> 8) & 0xFF));
  msg.push(uint8_t((val >> 0) & 0xFF));

  return msg;
}

MessageBuffer&
operator>>(MessageBuffer& msg, uintVar_t& v)
{
  uint8_t byte[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t first = msg.front();

  if ((first & (0x80)) == 0) {
    msg >> byte[0];
    uint8_t val = ((byte[0] & 0x7F) << 0);
    v = to_varint(val);
    return msg;
  }

  if ((first & (0x80 | 0x40)) == 0x80) {
    msg >> byte[1];
    msg >> byte[0];
    uint16_t val = (((uint16_t)byte[1] & 0x3F) << 8) + ((uint16_t)byte[0] << 0);
    v = to_varint(val);
    return msg;
  }

  if ((first & (0x80 | 0x40 | 0x20)) == (0x80 | 0x40)) {
    msg >> byte[3];
    msg >> byte[2];
    msg >> byte[1];
    msg >> byte[0];
    uint32_t val = ((uint32_t)(byte[3] & 0x1F) << 24) +
                   ((uint32_t)byte[2] << 16) + ((uint32_t)byte[1] << 8) +
                   ((uint32_t)byte[0] << 0);
    v = to_varint(val);
    return msg;
  }

  msg >> byte[7];
  msg >> byte[6];
  msg >> byte[5];
  msg >> byte[4];
  msg >> byte[3];
  msg >> byte[2];
  msg >> byte[1];
  msg >> byte[0];
  uint64_t val = ((uint64_t)(byte[7] & 0x0F) << 56) +
                 ((uint64_t)(byte[6]) << 48) + ((uint64_t)(byte[5]) << 40) +
                 ((uint64_t)(byte[4]) << 32) + ((uint64_t)(byte[3]) << 24) +
                 ((uint64_t)(byte[2]) << 16) + ((uint64_t)(byte[1]) << 8) +
                 ((uint64_t)(byte[0]) << 0);
  v = to_varint(val);
  return msg;
}

} // namespace quicr::messages
} // namespace quicr
