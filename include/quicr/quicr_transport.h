#include <optional>
#include <unistd.h>

namespace quicr {

///
/// Common Defines
///
/**
 * Return status
 */
enum class TransportReturnStatus
{
  Success = 0,
  SocketNotOpened,
  UnknownError,
  InvalidHostname,
  NotConnected,
  ConnectionError,
  InvalidFlowId,
  InvalidDestCid,
  ConnectionFailed,
};


/**
 * Transport destination IP information
 */
struct TransportDestination {
  std::string   hostname;  // Relay IP or FQDN
  uint16_t      port;      // Relay port to connect to
};

/**
 * Transport Configuration
 */
struct TransportConfig {
  TransportDestination dest;

  /* Other config, such as timeouts, TLS, ... */
};

/**
 * @brief Transport Interface
 *
 * @details IP transport interface class. QUIC is an implementation of
 *      this interface. This provides an interface for the underlining QUIC stack.
 *
 *      After instantiating class, perform the following:
 *
 *          myQuic.openSocket();
 *          myQuic.connect(<source CID>);
 *
 *      Followed by:
 *
 *          myQuic.writeTo();
 *          myQuic.readFrom();
 */
class QuicRTransport
{
  /**
   * Constructor
   *
   * @param config      The transport configuration to use
   */
  QuicRTransport(TransportConfig config) {
    sockFd = 0;
    socketReady = false;
    this->config = config;
  }

  /**
   * Deconstructor MUST clear/disconnect all connections and close socket.
   */
  virtual ~QuicRTransport() = default;

  /**
   * Opens the source UDP socket and sets socket options per config
   *
   * @returns TransportReturnStatus
   */
  virtual TransportReturnStatus openSocket() = 0;

  /**
   * Check if socket is opened and ready
   *
   * @return true if ready, false if not
   */
   bool isSocketReady() {
     return socketReady;
   }

   /**
   * Close socket
    */
   bool closeSocket() {
     if (sockFd > 0) {
       close(sockFd);
     }
   }

  /**
   * @brief Initiates a connection.
   *
   * @details initiates a new connection. This will be within the same tuple/IP flow.
   *        Connecting requires interaction with the destination in order to obtain
   *        the destination connection Id (dCid).  This is a blocking calling that will
   *        wait for that. A max timeout is defined in the config to this class.
   *        If timed out, the connection will indicate failed. Upon successful connection,
   *        the ```dCid``` will be updated.
   *
   * @param[in] sCid            Source connectionId to use for connection
   * @param[out] dCid           Destination connectionId. Will be updated with the final dCID to use.
   *
   * @returns TransportReturnStatus
   */
  virtual TransportReturnStatus connect(uint64_t sCid, uint64_t &dCid) = 0;

  /**
   * Disconnect a connection.
   *
   * @param[in] sCid            Source connectionId to disconnect/terminate
   *
   * @returns TransportReturnStatus
   */
  virtual TransportReturnStatus disconnect(uint64_t sCid) = 0;


  /**
   * Gets connection state
   *
   * @param[in] sCid            Source connectionId to disconnect/terminate
   *
   * @return True if connected, false if not connected
   */
  virtual bool isConnected(uint64_t sCid) {
    auto mapPtr = sourceCid_lum.find(sCid);

    if ( mapPtr != sourceCid_lum.end() ) {
      return &mapPtr->second->connected;
    }

    return false;
  }

  /**
   * @brief Write data to destination connection and flow Id
   *
   *
   * @details Writes data to destination connection Id and flow Id.
   *        Streams (reliable) and datagrams (unreliable) have a flowId.
   *
   *        Write is blocking, but likely will not be blocked due to flow control.
   *
   * @param data            Data to write. Size of vector defines the amount of data
   *                        to write.
   * @param dCid            Destination connection Id. This is the id that ties to
   *                        the destination IP/Port flow.
   * @param flowId          In QUIC, flowId equals streamId for streams. Datagrams
   *                        have a flowId as well. It's not really a stream, so it is
   *                        called flow instead. Regardless, items written to a flowId
   *                        can be re-ordered and made reliable as needed. It
   *                        represents a continual flow of data, such as a file.
   * @returns TransportReturnStatus
   */
  virtual TransportReturnStatus writeTo(const std::vector<uint8_t>& data,
                                        uint64_t dCid,
                                        uint64_t flowId) = 0;

  /**
   * @brief Write data to destination connection and flow Id with timeout in ms
   *
   * @details Same as WriteTo() without timeout. This method will block as long
   *    as timeout in milliseconds.
   *
   * @param timeoutMs      Milliseconds to block
   *
   * @returns TransportReturnStatus
   */
  virtual TransportReturnStatus writeTo(const std::vector<uint8_t>& data,
                                        uint64_t dCid,
                                        uint64_t flowId,
                                        uint16_t timeoutMs) = 0;

  /**
   * @brief Read from connection source Id and flowId into buffer
   *
   * @details Method will read from source connectionId and flowId.
   *
   *    Read is blocking.
   *
   * @param data            Data storage for read data. Length of vector defines how
   *                        much data can be read. Subsequent calls to this method
   *                        should be called to read more data if the buffer isn't
   *                        large enough.
   * @param sCid            Source connectionId to read from
   * @param flowId          flowId to read from
   *
   * @return number of bytes read.  Zero indicates no error, -1 indicates connection error
   */
  virtual int readFrom(std::vector<uint8_t>& data,
                       uint64_t sCid,
                       uint64_t flowId) = 0;

  /**
   * @brief Read from connection source Id and flowId into buffer with timeout
   *
   * @details Same as readFrom() with timeout in milliseconds
   *
   * @param timeoutMs      Milliseconds to block
   *
   * @return number of bytes read.  Zero indicates timeout, -1 indicates connection error
   */
  virtual int readFrom(std::vector<uint8_t>& data,
                       uint64_t sCid,
                       uint64_t flowId,
                       uint16_t timeoutMs) = 0;

  /**
   * Get Config
   *
   * @return copy of the configuration
   */
  TransportConfig getConfig() {
    return config;
  }

private:

  int sockFd;             // UDP socket FD
  bool socketReady;       // Indicates if the UDP socket is opened and ready

  /*
   * Source Connection Id structure.
   */
  struct sourceCid {
    uint64_t  sCid;        // Value of the source CID
    bool      connected;   // Indicates if connection is established or not
    uint16_t  seq;         // Index value that this CID is associated
  };

  TransportConfig config;

  /*
   * Below maps are lookup maps ("_lum") for source CID. The key is used to lookup the source CID.
   * These maps are maintained by connect() and disconnect() methods.
   */
  std::map<uint64_t, sourceCid*>  destCid_lum;     // Destination CID map to source CID. Used to track destination CIDs to source CIDs
  std::map<uint64_t, sourceCid*>  sourceCid_lum;   // Source CID lookup map. Key is the source CID mapping to struct value

  // Below map is the root source for the source CID, keyed by the sequence number
  //    A vector/array is not used since connection IDs come and go over the life of a socket/connection
  std::map<uint16_t, sourceCid>   sourceCid;      // Map of source connection Ids, key is the Cid sequence number.
  uint16_t curSrcCidSeq;                          // Current/latest source Cid sequence Id

};

}