#pragma once

#include <optional>
#include <string>
#include <vector>

namespace quicr
{
// TODO: Do we need a different structure or the name
using bytes = std::vector<uint8_t>;

/*
 * Context information managed by the underlying QUICR Stack
 * Applications get the QUICRContextId and pass same for 
 * as part of the API operations.
*/
using QUICRContext = uint64_t;



/**
 * Published media objects are uniquely identified with QUICRName.
 * The construction and the intepretation of the bits are
 * application specific. QUICR protocol and API must consider
 * these bits as opaque
  *
  *   Example:
  *
  * ~~~~~{.cpp}
  *      QuicRNameId nameId;
  *      nameId.length = 120;
  *      nameId.value.asNum.hi = 0xF000000000000000;
  *      nameId.value.asNum.low = 0x0000000000000001;
  *      nameId.value.asNum.hi >>=12;
  *      nameId.value.asNum.low |= 0x8000000000000000;
  *
  *      nameId.makeNbo();
  *      nameId.makeNbo();
  *      for (int i=0; i < 16; i++) {
  *          if (i == 8) printf(" -");
  *          printf(" %02X", nameId.value.asBytes[i]);
  *      }
  * ~~~~~
 */
class QuicRName {
private:
  bool bigEndian;

  union Value {
    struct {
      uint64_t hi;    // Little endian, use htonll() to set
      uint64_t low;   // Little endian, use htonll() to set
    } __attribute__ ((__packed__, __aligned__(1))) asNum;

    uint8_t asBytes[16];
  } __attribute__ ((__packed__, __aligned__(1)));

public:
  Value     value;      // The value of the name Id
  uint8_t   length;     // Number of significant bits (big-endian) of hi + low bits.  0 - 128

  QuicRName () : bigEndian(false) {};

  /**
   * Checks if the name is Big or Little endian encoded
   *
   * @return true if big endian, false if little endian
   */
  bool isBigEndian() {
    return bigEndian;
  }

  /**
   * Encode/Decode nameId in Network Byte Order
   */
  void makeNbo() {
    if (not bigEndian) {
      value.asNum.hi = htonll(value.asNum.hi);
      value.asNum.low = htonll(value.asNum.low);
      bigEndian = true;
    }
  }

  /**
   * Encode/Decode nameId in Host Byte Order
   */
  void makeHbo() {
    if (bigEndian) {
      value.asNum.hi = ntohll(value.asNum.hi);
      value.asNum.low = ntohll(value.asNum.low);
      bigEndian = false;
    }
  }
};

/**
 *  QUICRNamespace identifies set of possible QUICRNames 
 *  The mask length captures the length of bits, up to 128 bits, that are
 *  significant. Non-significant bits are ignored. In this sense, 
 *  a namespace is like an IPv6 prefix/len
 */
struct QUICRNamespace
{
  QuicRName name;
  size_t mask{ 0 }; // Number of significant bits (big-endian) of hi + low bits.  0 - 128
};


/** 
 * Hint providing the start point to serve a subscrption request.
 * Relays use this information to determine the start-point and 
 * serve the objects in the time-order from the cache.
*/
enum class SubscribeIntent
{
  immediate = 0, // Start from the most recent object
  wait_up = 1,   // Start from the following group
  sync_up = 2,   // Start from the request position
};

/** 
 * RelayInfo defines the connection information for relays
 */
struct RelayInfo {
  std::string   hostname;  // Relay IP or FQDN
  uint16_t      port;      // Relay port to connect to
};

/**
 * SubscribeResult defines the result of a subscription request
 */
struct SubscribeResult {

  enum class SubscribeStatus
  {
      Ok = 0,       // Success 
      Expired,      // Indicates the subscription is considered expired, anti-replay or otherwise
      Redirect,     // Not failed, this request should be reattempted to another relay as indicated
      FailedError,  // Failed due to relay error, error will be indicated
      FailedAuthz,  // Valid credentials, but not authorized
      TimeOut       // Timed out. This happens if failed auth or if there is a failure with the relay
                    //   Auth failures are timed out because providing status of failed auth can be exploited
  };

  SubscribeStatus status;         // Subscription status
  std::string reason_string;
  std::optional<uint64_t> subscriber_expiry_interval;
  RelayInfo       redirectInfo;   // Set only if status is redirect 
};

/**
 * Publish intent and message status
 */
enum class PublishStatus
{
  Ok = 0,         // Success 
  Redirect,       // Indicates the publish (intent or msg) should be reattempted to another relay
  FailedError,    // Failed due to relay error, error will be indicated
  FailedAuthz,    // Valid credentials, but not authorized
  ReAssigned,     // Publish intent is ok, but name/len has been reassigned due to restrictions.
  TimeOut         // Timed out. The relay failed or auth failed. 
};

/**
 * PublishIntentResult defines the result of a publish intent
 */
struct PublishIntentResult
{
  PublishStatus status;         // Publish status
  RelayInfo     redirectInfo;   // Set only if status is redirect
  QUICRName     reassignedName; // Set only if status is ReAssigned
};

/**
 * PublishMsgResult defines the result of a publish message
 */
struct PublishMsgResult
{
  PublishStatus status;         // Publish status
};

}