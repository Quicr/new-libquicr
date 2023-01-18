#pragma once

#include <optional>
#include <string>
#include <vector>

#include "transport.h"

namespace quicr {

// TODO: Do we need a different structure or the name
using bytes = std::vector<uint8_t>;

/**
 * QuicRNameId is the published name that identifies a set of subscribers.
 * 
 *   While this value is opaque to the relays, it is used by origins for authorization. 
 *   The name and origin servers are application/deployment specific.  
 *   
 *   The name Id conforms to the following: 
 * 
 *   * Is represented as two 64bit unsigned numbers, for a total of 128 bits
 *   * Little endian is used for in-code but transmission/wire is big endian
 *   * Does not have to be unique (sequence/message number is not required)
 *   * Can be reused, but must be authorized via the publish intent process
 *   * Length of the name defines the length of bits, up to 128 bits, that are
 *     significant. Non-significant bits are ignored. In this sense, a name Id/length
 *     is like an IPv6 prefix/len
 *  
 *   The length of the name id defines the significant bits (big endian). A published
 *    message is always 128 bits in length, which is represented with a length of 128.
 *    Considering a published message is always 128 bits in length, the length is ignored
 *    when publishing. 
 * 
 *   The length is use for both subscription and publish intent requests. Both define a
 *    length that in effect becomes a wildcard for what is being subscribed to or what
 *    is being authorized to publish to. Both subscribe and publish intent
 *    (aka request/authorization) will determine if the length is a valid length for
 *    a given name id. 
 * 
 *   Name Ids should be padded with unset bits (zero value) for intent message. Set bits
 *    longer than the length will be ignored/truncated.
 * 
 *   The format/schema of the encoded bits in the name Id are application specific. The
 *    primary requirement for names are that they must be encoded as big-endian bit values
 *    with a length value that indicates the significant bits that will be used to match
 *    a set of subscribers and publish intent (authorizations) requests.
 *
 * @example Usage:
 *  QuicRNameId nameId;
 *  nameId.length = 120;
 *  nameId.value.asNum.hi = 0xF000000000000000;
 *  nameId.value.asNum.low = 0x0000000000000001;
 *  nameId.value.asNum.hi >>=12;
 *  nameId.value.asNum.low |= 0x8000000000000000;
 *
 *  nameId.makeNbo();
 *  nameId.makeNbo();

 *  for (int i=0; i < 16; i++) {
 *    if (i == 8) printf(" -");
 *    printf(" %02X", nameId.value.asBytes[i]);
 *  }
*/
class QuicRNameId {
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

  QuicRNameId () : bigEndian(false) {};

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
 * SubscribeJoinMode enum defines the join mode for a new or resumed subscription 
 */
enum class SubscribeJoinMode
{
  Immediate = 0,  // Deliver new messages after subscription; no delay
  WaitNextMsg,    // Wait for next complete message; mid stream fragments are not transmitted
  LastX,          // Deliver the last X value number of complete messages and then deliver real-time
  Resume          // Deliver messages based on last delivered for the given session Id; resume after disconnect
                  //   If this is a first seen session, then treat as immediate. If this is for an existing
                  //   session, then resume as far back as the buffer allows, up to where the last message was
                  //   delivered.  This does require some level of state to track last message delivered for a
                  //   given session. 
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
  uint64_t      publishId;      // ID to use when publishing messages

  RelayInfo     redirectInfo;   // Set only if status is redirect
  QuicRNameId   reassignedName; // Set only if status is ReAssigned
};

/**
 * PublishMsgResult defines the result of a publish message
 */
struct PublishMsgResult
{
  PublishStatus status;         // Publish status
};
}