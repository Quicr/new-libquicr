#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common.h"
#include "transport.h"

namespace quicr {

/**
 * @brief Subscriber delegate callback methods
 * 
 * @note
 *  Fragments are handled by the library/implementation.  The client implementing
 *   this API always receives complete messages unless specifically
 *   requested during subscription to receive fragmented messgaes.
 *
 *   TTL of a message is indicated by the producer. When fragments are delivered,
 *   the TTL of the message will be the lowest/oldest seen of fragments.
 */
class SubscriberDelegate
{
public:
  virtual ~SubscriberDelegate() = default;

  /**
   * @brief Subscription response callback
   * 
   * @details This callback will be called when a subscription response
   *   is received (on error, timeout, ..).
   *
   * @param[in]  name           Name Id of the subscribe namedId, length will be set

   * @param[in]  result         Subscription result of the subscribe request
   */
  virtual void subscribeResponse(const QuicRNameId& name,
                                 const SubscribeResult& result) = 0;

  /**
   * @brief Subscription close callback
   *
   * @details Called when the subscription has ended or is to be closed.
   * 
   * @param[in] name    Name Id of the subscribe request
   */
  virtual void subscribeClose(const QuicRNameId& name) = 0;

  /**
   * @brief Received published message callback
   *
   * @details Called when messages are received. Messages can be buffered and
   *   deduplicated using the publisher Id and sequence Id.  Sequence Ids
   *   increment serially one at a time by publisher Id.  Publisher Id is an
   *   ephemeral unique number for a given time period based on publish intents.
   *
   * @param[in] name            Name Id of the published message; len will be 128.
   * @param[in] priority        Priority value for the message
   * @param[in] publishId       The publisher Id of the message
   * @param[in] seqId           Message sequence Id. This is relative per publisher Id
   * @param[in] data            Data of the message received
   */
  virtual void publishedMsg(const QuicRNameId& name,
                            uint8_t            priority,
                            uint64_t           publishId,
                            uint32_t           seqId,
                            bytes&&            data) = 0;

  /**
   * @brief Received published message callback for fragmented messages
   * 
   * @details same as onMsgRecv() but includes fragments.
   *   Fragments are only delivered to the caller via this callback if the
   *   subscription requests fragments.  Fragments are delivered using
   *   an index value, where 1 is the first message. The last message is
   *   indicated via `lastFragment`.
   *
   * @param[in] name            Name Id of the published message; len will be 128.
   * @param[in] priority        Priority value for the message
   * @param[in] publishId       The publisher Id of the message
   * @param[in] seqId           Message sequence Id. This is relative per publisher Id
   * @param[in] fragmentId      Zero if no fragments and is complete message,
   *                            otherwise fragment Id of the message.
   * @param[in] lastFragment    True if last fragment, false if not
   * @param[in] data            Data of the message received
   */
  virtual void publishedMsg(const QuicRNameId& name,
                            uint8_t            priority,
                            uint64_t           publishId,
                            uint32_t           seqId,
                            int                fragmentId,
                            bool               lastFragment,
                            bytes&&            data) = 0;
};

/**
 * @brief Publisher delegate callback methods
 *
 * @note
 *  Published messages are always complete messages. Fragmenting is handled by the library and
 *   transport implementation. Messages greater than MTU will automatically be split
 *   and fragmented via the pub/sub infrastructure.  
 */
class PublisherDelegate
{
public:
  virtual ~PublisherDelegate() = default;

  /**
   * @brief Published message ACK callback
   * 
   * @todo Support grouping of ACKs so that 
   *
   * @param[in] name      Published name ID being acknowledged. This is always 128 bit in length 
   * @param[in] publishId The publisher Id of the message
   * @param[in] seqId     Message sequence Id. This is relative per publisher Id
   * @param[in] result    Result of the publish operation
   */
  virtual void publishAck(const QuicRNameId&  name,
                            uint64_t            publishId,
                            const uint32_t      seqId,
                            PublishMsgResult&   result) = 0;

  /**
   * @brief Published intenet response callback
   *
   * @param[in] name      Original published name ID/len of publish intent
   * @param[in] result    Result of the publish operation
   */
  virtual void publishIntentResponse(const QuicRNameId&   name,
                                       PublishIntentResult& result) = 0;

};

/**
 * @brief Client API for using QuicR Protocol
 */
class QuicRClient
{
public:
  enum class ClientStatus {
      READY = 0,
      CONNECTING, 
      RELAY_HOST_INVALID,
      RELAY_PORT_INVALID,
      RELAY_NOT_CONNECTED,
      TRANSPORT_ERROR,
      UNAUTHORIZED,
      TERMINATED,
  };

  /**
   * @brief Construct a new QuicR Client
   *
   * @details A new client thread will be started with an event loop
   *   running to process received messages. Subscriber and publisher
   *   delegate callbacks will be called on received messages.
   *   The relay will be connected and maintained by the event loop.
   *
   * @param transport            QuicRTransport class implementation
   * @param subscriber_delegate  Subscriber delegate
   * @param pub_delegate         Publisher delegate
   */
  QuicRClient(const QuicRTransport &transport,
              SubscriberDelegate& subscriber_delegate,
              PublisherDelegate& pub_delegate);

  // Recvonly client
  QuicRClient(const QuicRTransport &transport,
              SubscriberDelegate& subscriber_delegate);

  // Sendonly client
  QuicRClient(const QuicRTransport &transport,
              PublisherDelegate& pub_delegate);


  /**
   * @brief Get the client status
   * 
   * @details This method should be used to determine if the client is
   *   connected and ready for publishing and subscribing to messages.
   *   Status will indicate the type of error if not ready. 
   * 
   * @returns client status
   */
  virtual ClientStatus status() = 0;

  /**
   * @brief Run client API event loop
   *
   * @details This method will connect to the relay/transport and run
   *    an event loop for calling the callbacks
   *
   * @returns client status
   */
  virtual ClientStatus run() = 0;

  /**
   * @brief Send Publish Intent
   * 
   * @details This method is asynchronous. The publisher delegate intent
   *    response method will be called to indicate intent status.
   * 
   * @details Express interest to publish media under a given QuicrName
   *    auth_token is used to validate the authorization for the
   *    name specified.
   *
   * @note (0): Intent to publish is typically done at a higher level
   *            grouping than individual objects.
   *            ex: user1/ or user1/cam1 or user1/space3/
   *            This ties authz to prefix/group rather than individual
   *            data objects.
   *
   * @note (1): Authorization Token shall embed the information
   *            needed for the authorizing entity to bind the name
   *            to the token.
   *
   * @todo Support array of names
   * 
   * @param name          Name ID/Len to request publish rights
   * @param useReliable   Request reliable transport for published messages
   * @param authToken     Authentication token for the origin
   * 
   * @return True if successful, false if not.  True only indicates
   *    that the message was sent to the relay. It does not indicate if it was
   *    accepted and authorized by the origin.  Publisher delegate
   *    is used for that. 
   */
  virtual bool publishIntent(const QuicRNameId& name,
                             bool  useReliable,
                             const std::string& authToken) = 0;

  /**
   * @brief Publish a message
   *
   * @details A message up to max message size is published. If the message is
   *   larger than MTU (minus overhead), the message will be fragmented.
   *
   * @param[in] name        Name ID to publish a message to. Length is 128
   * @param[in] priority    Priority value for the message
   * @param[in] ttl         Time to live value, represented as milliseconds
   *                        of age to be in cache
   * @param[in] publishId   The publisher Id from publish intent result
   * @param[in] seqId       Sequence Id to use, must be at least +1 from previous
   * @param[in] data        Data of the message to send. This can be up to max
   *                        message size. If the message is larger than MTU it will
   *                        be fragmented.
   * 
   * @return True if successful, false if not.  True only indicates
   *    that the message was sent to the relay. Publisher delegate
   *    is used to confirm ack/response from relay.
   */
  virtual bool publishMsg(const QuicRNameId& name,
                          uint8_t            priority,
                          uint32_t           ttl,
                          uint64_t           publishId,
                          uint32_t           seqId,
                          const bytes&       data) = 0;


  /**
   * @brief Send stop publish intent message
   *
   * @details Indicates that the publishing of messages is complete.
   *    The `publishId` will be marked finished. A new publish intent
   *    is required after sending a FIN. A new publish intent will have
   *    a new `publishId`
   * 
   * @param[in] name          Name ID/Len to request publish rights
   * @param[in] publishId The publisher Id of the message
   * @param[in] authToken    Authentication token for the origin
   */
  virtual void publishIntentFin(const QuicRNameId& name,
                                uint64_t publishId,
                                const std::string& authToken) = 0;

  /**
   * @brief Subscribe to given QuicRName/len
   * 
   * @param name             Name ID/Len to subscribe
   * @param intent           Join mode on start of subscription
   * @param useReliable      Request reliable transport for published messages
   * @param acceptFragments  True to configure the library to deliver message
   *                         fragments
   * @param authToken        Authentication token for the origin
   * 
   * @return True if successful, false if not.  True only indicates
   *    that the message was sent to the relay. It does not indicate if it was
   *    accepted and authorized.  Subscriber delegate is used for that. 
   */
  virtual bool subscribe(const QuicRNameId& name,
                         SubscribeJoinMode& joinMode,
                         bool               useReliable,
                         bool               acceptFragments,
                         const std::string& authToken) = 0;


  /**
   * @brief Unsubscribe to given QuicRName/len
   * 
   * @param name          Name ID/Len to unsubscribe. Must match the subscription
   * @param auth_token    Authentication token for the origin
   */
  virtual void unsubscribe(const QuicRNameId& name, const std::string& auth_token) = 0;


};

}