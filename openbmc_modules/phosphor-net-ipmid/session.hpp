#pragma once

#include "auth_algo.hpp"
#include "crypt_algo.hpp"
#include "endian.hpp"
#include "integrity_algo.hpp"
#include "prng.hpp"
#include "socket_channel.hpp"

#include <chrono>
#include <exception>
#include <ipmid/api.hpp>
#include <ipmid/sessiondef.hpp>
#include <list>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/object.hpp>
#include <string>
#include <unordered_map>
#include <user_channel/channel_layer.hpp>
#include <user_channel/user_layer.hpp>
#include <vector>
#include <xyz/openbmc_project/Ipmi/SessionInfo/server.hpp>

namespace session
{

using namespace std::chrono_literals;
using SessionID = uint32_t;

enum class Privilege : uint8_t
{
    HIGHEST_MATCHING,
    CALLBACK,
    USER,
    OPERATOR,
    ADMIN,
    OEM,
};

// Seconds of inactivity allowed during session setup stage
constexpr auto SESSION_SETUP_TIMEOUT = 5s;
// Seconds of inactivity allowed when session is active
constexpr auto SESSION_INACTIVITY_TIMEOUT = 60s;

// Mask to get only the privilege from requested maximum privlege (RAKP message
// 1)
constexpr uint8_t reqMaxPrivMask = 0xF;

/**
 * @struct SequenceNumbers Session Sequence Numbers
 *
 * IPMI v2.0 RMCP+ Session Sequence Numbers are used for rejecting packets that
 * may have been duplicated by the network or intentionally replayed. There are
 * two sets of Session SequenceNumbers for a given session.One set of inbound
 * and outbound sequence numbers is used for authenticated (signed) packets,
 * and the other set is used for unauthenticated packets.
 *
 * The individual Session Sequence Numbers is are initialized to zero whenever
 * a session is created and incremented by one at the start of outbound
 * processing for a given packet (i.e. the first transmitted packet has a ‘1’
 * as the sequence number, not 0). Session Sequence numbers are incremented for
 * every packet that is transmitted by a given sender, regardless of whether
 * the payload for the packet is a ‘retry’ or not.
 */
struct SequenceNumbers
{
    auto get(bool inbound = true) const
    {
        return inbound ? in : out;
    }

    void set(uint32_t seqNumber, bool inbound = true)
    {
        inbound ? (in = seqNumber) : (out = seqNumber);
    }

    auto increment()
    {
        return ++out;
    }

  private:
    uint32_t in = 0;
    uint32_t out = 0;
};
/**
 * @class Session
 *
 * Encapsulates the data related to an IPMI Session
 *
 * Authenticated IPMI communication to the BMC is accomplished by establishing
 * a session. Once established, a session is identified by a Session ID. The
 * Session ID may be thought of as a handle that identifies a connection between
 * a given remote user and the BMC. The specification supports having multiple
 * active sessions established with the BMC. It is recommended that a BMC
 * implementation support at least four simultaneous sessions
 */

using SessionIface = sdbusplus::server::object::object<
    sdbusplus::xyz::openbmc_project::Ipmi::server::SessionInfo>;

class Session : public SessionIface
{
  public:
    Session() = default;
    ~Session() = default;
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = default;
    Session& operator=(Session&&) = default;

    /**
     * @brief Session Constructor
     *
     * This is issued by the Session Manager when a session is started for
     * the Open SessionRequest command
     *
     * @param[in] inRemoteConsoleSessID - Remote Console Session ID
     * @param[in] priv - Privilege Level requested in the Command
     */
    Session(sdbusplus::bus::bus& bus, const char* path,
            SessionID inRemoteConsoleSessID, SessionID BMCSessionID,
            char priv) :
        SessionIface(bus, path)
    {
        reqMaxPrivLevel = static_cast<session::Privilege>(priv);
        bmcSessionID = BMCSessionID;
        remoteConsoleSessionID = inRemoteConsoleSessID;
    }

    auto getBMCSessionID() const
    {
        return bmcSessionID;
    }

    auto getRCSessionID() const
    {
        return remoteConsoleSessionID;
    }

    auto getAuthAlgo() const
    {
        if (authAlgoInterface)
        {
            return authAlgoInterface.get();
        }
        else
        {
            throw std::runtime_error("Authentication Algorithm Empty");
        }
    }

    void setAuthAlgo(std::unique_ptr<cipher::rakp_auth::Interface>&& inAuthAlgo)
    {
        authAlgoInterface = std::move(inAuthAlgo);
    }

    /**
     * @brief Get Session's Integrity Algorithm
     *
     * @return pointer to the integrity algorithm
     */
    auto getIntegrityAlgo() const
    {
        if (integrityAlgoInterface)
        {
            return integrityAlgoInterface.get();
        }
        else
        {
            throw std::runtime_error("Integrity Algorithm Empty");
        }
    }

    /**
     * @brief Set Session's Integrity Algorithm
     *
     * @param[in] integrityAlgo - unique pointer to integrity algorithm
     *                              instance
     */
    void setIntegrityAlgo(
        std::unique_ptr<cipher::integrity::Interface>&& integrityAlgo)
    {
        integrityAlgoInterface = std::move(integrityAlgo);
    }

    /** @brief Check if integrity algorithm is enabled for this session.
     *
     *  @return true if integrity algorithm is enabled else false.
     */
    auto isIntegrityAlgoEnabled()
    {
        return integrityAlgoInterface ? true : false;
    }

    /**
     * @brief Get Session's Confidentiality Algorithm
     *
     * @return pointer to the confidentiality algorithm
     */
    auto getCryptAlgo() const
    {
        if (cryptAlgoInterface)
        {
            return cryptAlgoInterface.get();
        }
        else
        {
            throw std::runtime_error("Confidentiality Algorithm Empty");
        }
    }

    /**
     * @brief Set Session's Confidentiality Algorithm
     *
     * @param[in] confAlgo - unique pointer to confidentiality algorithm
     *                       instance
     */
    void setCryptAlgo(std::unique_ptr<cipher::crypt::Interface>&& cryptAlgo)
    {
        cryptAlgoInterface = std::move(cryptAlgo);
    }

    /** @brief Check if confidentiality algorithm is enabled for this
     *         session.
     *
     *  @return true if confidentiality algorithm is enabled else false.
     */
    auto isCryptAlgoEnabled()
    {
        return cryptAlgoInterface ? true : false;
    }

    void updateLastTransactionTime()
    {
        lastTime = std::chrono::steady_clock::now();
    }

    /**
     * @brief Session Active Status
     *
     * Session Active status is decided upon the Session State and the last
     * transaction time is compared against the session inactivity timeout.
     *
     */
    bool isSessionActive(uint8_t sessionState)
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(
            currentTime - lastTime);

        State state = static_cast<session::State>(sessionState);

        switch (state)
        {
            case State::setupInProgress:
                if (elapsedSeconds < SESSION_SETUP_TIMEOUT)
                {
                    return true;
                }
                break;
            case State::active:
                if (elapsedSeconds < SESSION_INACTIVITY_TIMEOUT)
                {
                    return true;
                }
                break;
            default:
                return false;
        }
        return false;
    }

    /**
     * @brief Session's Requested Maximum Privilege Level
     */
    Privilege reqMaxPrivLevel;

    /**
     * @brief session's user & channel access details
     */
    ipmi::PrivAccess sessionUserPrivAccess{};
    ipmi::ChannelAccess sessionChannelAccess{};

    SequenceNumbers sequenceNums; // Session Sequence Numbers
    std::string userName{};       // User Name

    /** @brief Socket channel for communicating with the remote client.*/
    std::shared_ptr<udpsocket::Channel> channelPtr;

  private:
    SessionID bmcSessionID = 0;           // BMC Session ID
    SessionID remoteConsoleSessionID = 0; // Remote Console Session ID

    // Authentication Algorithm Interface for the Session
    std::unique_ptr<cipher::rakp_auth::Interface> authAlgoInterface;

    // Integrity Algorithm Interface for the Session
    std::unique_ptr<cipher::integrity::Interface> integrityAlgoInterface =
        nullptr;

    // Confidentiality Algorithm Interface for the Session
    std::unique_ptr<cipher::crypt::Interface> cryptAlgoInterface = nullptr;

    // Last Transaction Time
    decltype(std::chrono::steady_clock::now()) lastTime;
};

} // namespace session
