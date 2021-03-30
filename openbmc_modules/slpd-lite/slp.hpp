#pragma once

#include "slp_service_info.hpp"

#include <stdio.h>

#include <array>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace slp
{

using buffer = std::vector<uint8_t>;

template <typename T>
using deleted_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

namespace request
{

/*
 * @struct ServiceType
 *
 * SLP Message structure for ServiceType Request.
 */
struct ServiceType
{
    std::string prList;
    std::string namingAuth;
    std::string scopeList;
};

/*
 * @struct Service
 *
 * SLP Message structure for Service Request.
 */
struct Service
{
    std::string prList;
    std::string srvType;
    std::string scopeList;
    std::string predicate;
    std::string spistr;
};
} // namespace request

/*
 * @enum FunctionType
 *
 * SLP Protocol supported Message types.
 */
enum class FunctionType : uint8_t
{
    SRVRQST = 0x01,
    SRVRPLY = 0x02,
    ATTRRQST = 0x06,
    ATTRRPLY = 0x07,
    SRVTYPERQST = 0x09,
    SRVTYPERPLY = 0x0A,
    SAADV = 0x0B,
};

/*
 * @enum Error
 *
 * SLP Protocol defined Error Codes.
 */
enum class Error : uint8_t
{
    LANGUAGE_NOT_SUPPORTED = 0x01,
    PARSE_ERROR = 0x02,
    INVALID_REGISTRATION = 0x03,
    SCOPE_NOT_SUPPORTED = 0x04,
    AUTHENTICATION_UNKNOWN = 0x05,
    AUTHENTICATION_ABSENT = 0x06,
    AUTHENTICATION_FAILED = 0x07,
    VER_NOT_SUPPORTED = 0x09,
    INTERNAL_ERROR = 0x0A,
    DA_BUSY_NOW = 0x0B,
    OPTION_NOT_UNDERSTOOD = 0x0C,
    INVALID_UPDATE = 0x0D,
    MSG_NOT_SUPPORTED = 0x0E,
};

/*
 * @struct Header
 *
 * SLP Protocol Header
 */
struct Header
{
    uint8_t version = 0;
    uint8_t functionID = 0;
    std::array<uint8_t, 3> length;
    uint16_t flags = 0;
    std::array<uint8_t, 3> extOffset;
    uint16_t xid = 0;
    uint16_t langtagLen = 0;
    std::string langtag;
};

/*
 * @struct Payload
 * This is a payload of the SLP Message currently
 * we are supporting two request.
 *
 */
struct Payload
{
    request::ServiceType srvtyperqst;
    request::Service srvrqst;
};

/*
 * @struct Message
 *
 * This will denote the slp Message.
 */
struct Message
{
    Header header;
    Payload body;
};

namespace parser
{

/** Parse a buffer and fill the header and the body of the message.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 *
 * @return Zero on success and parsed msg object,
 *         non-zero on failure and empty msg object.
 *
 */

std::tuple<int, Message> parseBuffer(const buffer& buf);

namespace internal
{

/** Parse header data from the buffer.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 *
 * @return Zero on success and fills header object inside message,
 *         non-zero on failure and empty msg object.
 *
 * @internal
 */

std::tuple<int, Message> parseHeader(const buffer& buf);

/** Parse a srvType request
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 *
 * @return Zero on success,and fills the body object inside message.
 *         non-zero on failure and empty msg object.
 *
 * @internal
 */

int parseSrvTypeRqst(const buffer& buf, Message& req);

/** Parse a service request.
 *
 * @param[in] buffer - The buffer from which data should be parsed.
 *
 * @return Zero on success,and fills the body object inside message.
 *         non-zero on failure and empty msg object.
 *
 * @internal
 */

int parseSrvRqst(const buffer& buf, Message& req);

} // namespace internal
} // namespace parser

namespace handler
{

/** Handle the  request  message.
 *
 * @param[in] msg - The message to process.
 *
 * @return In case of success, the vector is populated with the data
 *         available on the socket and return code is 0.
 *         In case of error, nonzero code and vector is set to size 0.
 *
 */

std::tuple<int, buffer> processRequest(const Message& msg);

/** Handle the error
 *
 * @param[in] msg - Req message.
 * @param[in] err - Error code.
 *
 * @return the vector populated with the error data
 */

buffer processError(const Message& req, const uint8_t err);
namespace internal
{

using ServiceList = std::map<std::string, slp::ConfigData>;
/** Handle the  SrvRequest message.
 *
 * @param[in] msg - The message to process
 *
 * @return In case of success, the vector is populated with the data
 *         available on the socket and return code is 0.
 *         In case of error, nonzero code and vector is set to size 0.
 *
 * @internal
 */

std::tuple<int, buffer> processSrvRequest(const Message& msg);

/** Handle the  SrvTypeRequest message.
 *
 * @param[in] msg - The message to process
 *
 * @return In case of success, the vector is populated with the data
 *         available on the socket and return code is 0.
 *         In case of error, nonzero code and vector is set to size 0.
 *
 * @internal
 *
 */

std::tuple<int, buffer> processSrvTypeRequest(const Message& msg);

/**  Read the SLPinfo from the configuration.
 *
 * @param[in] filename - Name of the conf file
 *
 * @return the list of the services
 *
 * @internal
 *
 */
ServiceList readSLPServiceInfo();

/**  Get all the interface address
 *
 * @return the list of the interface address.
 *
 * @internal
 *
 */

std::list<std::string> getIntfAddrs();

/** Fill the buffer with the header data from the request object
 *
 * @param[in] req - Header data will be copied from
 *
 * @return the vector is populated with the data
 *
 * @internal
 */
buffer prepareHeader(const Message& req);

} // namespace internal
} // namespace handler
} // namespace slp
