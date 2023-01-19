#pragma once

#include <optional>
#include <string>
#include <vector>

#include "common.h"
#include "transport.h"

namespace quicr {

/**
 * @brief Server/Relay callback methods
 */
class ServerDelegate
{
public:
  virtual ~ServerDelegate() = default;

  /**
   * @brief Received published message callback
   *
   * @details Called when messages are received. Messages can be buffered and
   *   deduplicated using the publisher Id and sequence Id.  Sequence Ids
   *   increment serially one at a time by publisher Id.  Publisher Id is an
   *   ephemeral unique number for a given time period based on publish intents.
   *
   *   Unlike the client implementation, the server always receives fragments.
   *   A message is always transmitted as a fragment via QuicR protocol. If
   *   the message fits within MTU, then the message fragment will be 1 with the
   *   FIN bit set. Zero fragment ID is treated as the same as 1 with lastFragment
   *   set to true.
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
  virtual void onPublishedMsg(const QuicRNameId& name,
                         uint8_t            priority,
                         uint64_t           publishId,
                         uint32_t           seqId,
                         int                fragmentId,
                         bool               lastFragment,
                         bytes&&            data) = 0;


  /**
   * @brief Subscribe request to given QuicRName/len
   *
   * @param name             Name ID/Len to subscribe
   * @param intent           Join mode on start of subscription
   * @param isReliable       Is using reliable transport for published messages
   * @param acceptFragments  True to allow sending fragments
   * @param authToken        Authentication token for the origin
   *
   */
  virtual void onSubscribeRequest(const QuicRNameId& name,
                                SubscribeJoinMode& joinMode,
                                bool               useReliable,
                                bool               acceptFragments,
                                const std::string& authToken) = 0;

  /**
   * @brief Unsubscription request callback
   *
   * @details Called when the client sends an unsubscription request
   *
   * @param[in] name    Name Id of the subscribe request to unsubscribe
   * @param auth_token    Authentication token for the origin
   */
  virtual void onUnsubscribeRequest(const QuicRNameId& name,
                                  const std::string& auth_token) = 0;


  /**
   * @brief Publish Intent request received callback
   *
   * @details Called when a publish intent message is received
   *
   * @param name          Name ID/Len to request publish rights
   * @param useReliable   Request reliable transport for published messages
   * @param authToken     Authentication token for the origin
   */
  virtual void onPublishIntentRequest(const QuicRNameId& name,
                                    bool  useReliable,
                                    const std::string& authToken) = 0;

  /**
   * @brief Publish intent FIN/close received callback
   *
   * @details Called when client sends publish intent FIN/close request.
   *
   * @param[in] name        Name ID/Len to request publish rights
   * @param[in] publishId   The publisher Id of the message
   * @param[in] authToken   Authentication token for the origin
   */
  void onPublishIntentFinRequest(const QuicRNameId& name,
                               uint64_t publishId,
                               const std::string& authToken);

};


/**
 * @brief Server API for using QuicR Protocol
 */
class QuicRServer
{
public:
  /**
   * @brief Construct a new QuicR Server
   *
   * @details A new server thread will be started with an event loop
   *   running to process received messages.
   *
   * @param transport            QuicRTransport class implementation
   * @param serverDelegate       Server delegate (callback) class to use
   */
  QuicRServer(const QuicRTransport transport,
              ServerDelegate& serverDelegate);


  /**
   * @brief Run Server API event loop
   *
   * @details This method will open listening sockets and run an event loop
   *    for callbacks.
   *
   * @returns true if error, false if no error
   */
  virtual bool run() = 0;

  /**
   * @brief Send publish intent ok
   *
   * @param[in] name        Name ID/len
   * @param[in] result      Result of the publish intent to send to client
   *
   * @return True if successful, false if not.  True only indicates
   *    that the message was sent.
   */
  virtual bool publishIntentOk(const QuicRNameId& name,
                               const PublishIntentResult& result) = 0;

  /**
   * @brief Send subscription result
   *
   * @param[in] name        Name ID/len
   * @param[in] result      Result of the subscription to send to client
   *
   * @return True if successful, false if not.  True only indicates
   *    that the message was sent.
   */
  virtual bool subscribeOk(const QuicRNameId& name,
                           const SubscribeResult& result) = 0;

  /**
   * @brief Subscription close/end
   *
   * @details Called when the subscription is to be closed or has ended.
   *
   * @param[in] name    Name Id of the subscribe request
   *
   * @return True if successful, false if not.  True only indicates
   *    that the message was sent.
   */
  virtual bool subscribeClose(const QuicRNameId& name) = 0;

  /**
   * @brief Send published message to client
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
  virtual void publishMsg(const QuicRNameId& name,
                          uint8_t            priority,
                          uint64_t           publishId,
                          uint32_t           seqId,
                          int                fragmentId,
                          bool               lastFragment,
                          bytes&&            data) = 0;
};

}