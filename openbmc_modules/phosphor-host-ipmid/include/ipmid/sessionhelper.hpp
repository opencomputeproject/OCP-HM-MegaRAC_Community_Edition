#include <sstream>
#include <string>

/**
 * @brief parse session input payload.
 *
 * This function retrives the session id and session handle from the session
 * object path.
 * A valid object path will be in the form
 * "/xyz/openbmc_project/ipmi/session/channel/sessionId_sessionHandle"
 *
 * Ex: "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a"
 * SessionId    : 0X12a4567d
 * SessionHandle: 0X8a

 * @param[in] objectPath - session object path
 * @param[in] sessionId - retrived session id will be asigned.
 * @param[in] sessionHandle - retrived session handle will be asigned.
 *
 * @return true if session id and session handle are retrived else returns
 * false.
 */
bool parseCloseSessionInputPayload(const std::string& objectPath,
                                   uint32_t& sessionId, uint8_t& sessionHandle)
{
    if (objectPath.empty())
    {
        return false;
    }
    // getting the position of session id and session handle string from
    // object path.
    std::size_t ptrPosition = objectPath.rfind("/");
    uint16_t tempSessionHandle = 0;

    if (ptrPosition != std::string::npos)
    {
        // get the sessionid & session handle string from the session object
        // path Ex: sessionIdString: "12a4567d_8a"
        std::string sessionIdString = objectPath.substr(ptrPosition + 1);
        std::size_t pos = sessionIdString.rfind("_");

        if (pos != std::string::npos)
        {
            // extracting the session handle
            std::string sessionHandleString = sessionIdString.substr(pos + 1);
            // extracting the session id
            sessionIdString = sessionIdString.substr(0, pos);
            // converting session id string  and session handle string to
            // hexadecimal.
            std::stringstream handle(sessionHandleString);
            handle >> std::hex >> tempSessionHandle;
            sessionHandle = tempSessionHandle & 0xFF;
            std::stringstream idString(sessionIdString);
            idString >> std::hex >> sessionId;
            return true;
        }
    }
    return false;
}

/**
 * @brief is session object matched.
 *
 * This function checks whether the objectPath contains reqSessionId and
 * reqSessionHandle, e.g., "/xyz/openbmc_project/ipmi/session/eth0/12a4567d_8a"
 * matches sessionId 0x12a4567d and sessionHandle 0x8a.
 *
 * @param[in] objectPath - session object path
 * @param[in] reqSessionId - request session id
 * @param[in] reqSessionHandle - request session handle
 *
 * @return true if the object is matched else return false
 **/
bool isSessionObjectMatched(const std::string objectPath,
                            const uint32_t reqSessionId,
                            const uint8_t reqSessionHandle)
{
    uint32_t sessionId = 0;
    uint8_t sessionHandle = 0;

    if (parseCloseSessionInputPayload(objectPath, sessionId, sessionHandle))
    {
        return (reqSessionId == sessionId) ||
               (reqSessionHandle == sessionHandle);
    }

    return false;
}
