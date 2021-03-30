#include "sessions_manager.hpp"

#include "main.hpp"
#include "session.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <user_channel/channel_layer.hpp>

using namespace phosphor::logging;

namespace session
{

static std::array<uint8_t, session::maxNetworkInstanceSupported>
    ipmiNetworkChannelNumList = {0};

void Manager::setNetworkInstance(void)
{

    uint8_t index = 0, ch = 1;
    // Constructing net-ipmid instances list based on channel info
    // valid channel start from 1 to 15  and assuming max 4 LAN channel
    // supported

    while (ch < ipmi::maxIpmiChannels &&
           index < session::maxNetworkInstanceSupported)
    {
        ipmi::ChannelInfo chInfo;
        ipmi::getChannelInfo(ch, chInfo);
        if (static_cast<ipmi::EChannelMediumType>(chInfo.mediumType) ==
            ipmi::EChannelMediumType::lan8032)
        {

            if (getInterfaceIndex() == ch)
            {
                ipmiNetworkInstance = index;
            }

            ipmiNetworkChannelNumList[index] = ch;
            index++;
        }
        ch++;
    }
}

uint8_t Manager::getNetworkInstance(void)
{
    return ipmiNetworkInstance;
}

Manager::Manager()
{
}

void Manager::managerInit(const std::string& channel)
{

    /*
     * Session ID is 0000_0000h for messages that are sent outside the session.
     * The session setup commands are sent on this session, so when the session
     * manager comes up, is creates the Session ID  0000_0000h. It is active
     * through the lifetime of the Session Manager.
     */

    objManager = std::make_unique<sdbusplus::server::manager::manager>(
        *getSdBus(), session::sessionManagerRootPath);

    auto objPath =
        std::string(session::sessionManagerRootPath) + "/" + channel + "/0";

    chName = channel;
    setNetworkInstance();
    sessionsMap.emplace(
        0, std::make_shared<Session>(*getSdBus(), objPath.c_str(), 0, 0, 0));
}

std::shared_ptr<Session>
    Manager::startSession(SessionID remoteConsoleSessID, Privilege priv,
                          cipher::rakp_auth::Algorithms authAlgo,
                          cipher::integrity::Algorithms intAlgo,
                          cipher::crypt::Algorithms cryptAlgo)
{
    std::shared_ptr<Session> session = nullptr;
    SessionID bmcSessionID = 0;
    cleanStaleEntries();
    uint8_t sessionHandle = 0;

    auto activeSessions = sessionsMap.size() - session::maxSessionlessCount;

    if (activeSessions < session::maxSessionCountPerChannel)
    {
        do
        {
            bmcSessionID = (crypto::prng::rand());
            bmcSessionID &= session::multiIntfaceSessionIDMask;
            // In sessionID , BIT 31 BIT30 are used for netipmid instance
            bmcSessionID |= ipmiNetworkInstance << 30;
            /*
             * Every IPMI Session has two ID's attached to it Remote Console
             * Session ID and BMC Session ID. The remote console ID is passed
             * along with the Open Session request command. The BMC session ID
             * is the key for the session map and is generated using std::rand.
             * There is a rare chance for collision of BMC session ID, so the
             * following check validates that. In the case of collision the
             * created session is reset and a new session is created for
             * validating collision.
             */
            auto iterator = sessionsMap.find(bmcSessionID);
            if (iterator != sessionsMap.end())
            {
                // Detected BMC Session ID collisions
                continue;
            }
            else
            {
                break;
            }
        } while (1);

        sessionHandle = storeSessionHandle(bmcSessionID);

        if (!sessionHandle)
        {
            throw std::runtime_error(
                "Invalid sessionHandle - No sessionID slot ");
        }
        sessionHandle &= session::multiIntfaceSessionHandleMask;
        // In sessionID , BIT 31 BIT30 are used for netipmid instance
        sessionHandle |= ipmiNetworkInstance << 6;
        std::stringstream sstream;
        sstream << std::hex << bmcSessionID;
        std::stringstream shstream;
        shstream << std::hex << (int)sessionHandle;
        auto objPath = std::string(session::sessionManagerRootPath) + "/" +
                       chName + "/" + sstream.str() + "_" + shstream.str();
        session = std::make_shared<Session>(*getSdBus(), objPath.c_str(),
                                            remoteConsoleSessID, bmcSessionID,
                                            static_cast<uint8_t>(priv));

        // Set the Authentication Algorithm
        switch (authAlgo)
        {
            case cipher::rakp_auth::Algorithms::RAKP_HMAC_SHA1:
            {
                session->setAuthAlgo(
                    std::make_unique<cipher::rakp_auth::AlgoSHA1>(intAlgo,
                                                                  cryptAlgo));
                break;
            }
            case cipher::rakp_auth::Algorithms::RAKP_HMAC_SHA256:
            {
                session->setAuthAlgo(
                    std::make_unique<cipher::rakp_auth::AlgoSHA256>(intAlgo,
                                                                    cryptAlgo));
                break;
            }
            default:
            {
                throw std::runtime_error("Invalid Authentication Algorithm");
            }
        }

        sessionsMap.emplace(bmcSessionID, session);
        session->sessionHandle(sessionHandle);

        return session;
    }

    log<level::INFO>("No free RMCP+ sessions left");

    throw std::runtime_error("No free sessions left");
}

bool Manager::stopSession(SessionID bmcSessionID)
{
    auto iter = sessionsMap.find(bmcSessionID);
    if (iter != sessionsMap.end())
    {
        iter->second->state(
            static_cast<uint8_t>(session::State::tearDownInProgress));
        return true;
    }
    else
    {
        return false;
    }
}

std::shared_ptr<Session> Manager::getSession(SessionID sessionID,
                                             RetrieveOption option)
{
    switch (option)
    {
        case RetrieveOption::BMC_SESSION_ID:
        {
            auto iter = sessionsMap.find(sessionID);
            if (iter != sessionsMap.end())
            {
                return iter->second;
            }
            break;
        }
        case RetrieveOption::RC_SESSION_ID:
        {
            auto iter = std::find_if(
                sessionsMap.begin(), sessionsMap.end(),
                [sessionID](
                    const std::pair<const uint32_t, std::shared_ptr<Session>>&
                        in) -> bool {
                    return sessionID == in.second->getRCSessionID();
                });

            if (iter != sessionsMap.end())
            {
                return iter->second;
            }
            break;
        }
        default:
            throw std::runtime_error("Invalid retrieval option");
    }

    throw std::runtime_error("Session ID not found");
}

void Manager::cleanStaleEntries()
{
    for (auto iter = sessionsMap.begin(); iter != sessionsMap.end();)
    {

        auto session = iter->second;
        if ((session->getBMCSessionID() != session::sessionZero) &&
            !(session->isSessionActive(session->state())))
        {
            sessionHandleMap[getSessionHandle(session->getBMCSessionID())] = 0;
            iter = sessionsMap.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

uint8_t Manager::storeSessionHandle(SessionID bmcSessionID)
{
    // Handler index 0 is  reserved for invalid session.
    // index starts with 1, for direct usage. Index 0 reserved
    for (uint8_t i = 1; i <= session::maxSessionCountPerChannel; i++)
    {
        if (sessionHandleMap[i] == 0)
        {
            sessionHandleMap[i] = bmcSessionID;
            return i;
        }
    }
    return 0;
}

uint32_t Manager::getSessionIDbyHandle(uint8_t sessionHandle) const
{
    if (sessionHandle <= session::maxSessionCountPerChannel)
    {
        return sessionHandleMap[sessionHandle];
    }
    return 0;
}

uint8_t Manager::getSessionHandle(SessionID bmcSessionID) const
{

    // Handler index 0 is reserved for invalid session.
    // index starts with 1, for direct usage. Index 0 reserved

    for (uint8_t i = 1; i <= session::maxSessionCountPerChannel; i++)
    {
        if (sessionHandleMap[i] == bmcSessionID)
        {
            return (i);
        }
    }
    return 0;
}
uint8_t Manager::getActiveSessionCount() const
{

    return (std::count_if(
        sessionsMap.begin(), sessionsMap.end(),
        [](const std::pair<const uint32_t, std::shared_ptr<Session>>& in)
            -> bool {
            return in.second->state() ==
                   static_cast<uint8_t>(session::State::active);
        }));
}
} // namespace session
