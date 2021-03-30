#include "endian.hpp"
#include "slp.hpp"
#include "slp_meta.hpp"

#include <string.h>

#include <algorithm>
#include <string>

namespace slp
{

namespace parser
{

namespace internal
{

std::tuple<int, Message> parseHeader(const buffer& buff)
{
    /*  0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    Version    |  Function-ID  |            Length             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       | Length, contd.|O|F|R|       reserved          |Next Ext Offset|
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |  Next Extension Offset, contd.|              XID              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      Language Tag Length      |         Language Tag          \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

    Message req{0};
    int rc = slp::SUCCESS;

    std::copy_n(buff.data(), slp::header::SIZE_VERSION, &req.header.version);

    std::copy_n(buff.data() + slp::header::OFFSET_FUNCTION,
                slp::header::SIZE_VERSION, &req.header.functionID);

    std::copy_n(buff.data() + slp::header::OFFSET_LENGTH,
                slp::header::SIZE_LENGTH, req.header.length.data());

    std::copy_n(buff.data() + slp::header::OFFSET_FLAGS,
                slp::header::SIZE_FLAGS, (uint8_t*)&req.header.flags);

    req.header.flags = endian::from_network(req.header.flags);
    std::copy_n(buff.data() + slp::header::OFFSET_EXT, slp::header::SIZE_EXT,
                req.header.extOffset.data());

    std::copy_n(buff.data() + slp::header::OFFSET_XID, slp::header::SIZE_XID,
                (uint8_t*)&req.header.xid);

    req.header.xid = endian::from_network(req.header.xid);

    uint16_t langtagLen;

    std::copy_n(buff.data() + slp::header::OFFSET_LANG_LEN,
                slp::header::SIZE_LANG, (uint8_t*)&langtagLen);

    langtagLen = endian::from_network(langtagLen);

    req.header.langtag.insert(
        0, (const char*)buff.data() + slp::header::OFFSET_LANG, langtagLen);

    /* check for the validity of the function */
    if (req.header.functionID <
            static_cast<uint8_t>(slp::FunctionType::SRVRQST) ||
        req.header.functionID > static_cast<uint8_t>(slp::FunctionType::SAADV))
    {
        rc = static_cast<int>(slp::Error::PARSE_ERROR);
    }

    return std::make_tuple(rc, std::move(req));
}

int parseSrvTypeRqst(const buffer& buff, Message& req)
{

    /*  0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |        length of PRList       |        <PRList> String        \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |   length of Naming Authority  |   <Naming Authority String>   \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |     length of <scope-list>    |      <scope-list> String      \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

    /* Enforce SLPv2 service type request size limits. */
    if (buff.size() < slp::request::MIN_SRVTYPE_LEN)
    {
        return (int)slp::Error::PARSE_ERROR;
    }

    /* Parse the PRList. */
    uint16_t prListLen;
    std::copy_n(buff.data() + slp::request::OFFSET_PR_LEN,
                slp::request::SIZE_PRLIST, (uint8_t*)&prListLen);

    prListLen = endian::from_network(prListLen);

    req.body.srvtyperqst.prList.insert(
        0, (const char*)buff.data() + slp::request::OFFSET_PR, prListLen);

    uint8_t pos = slp::request::OFFSET_PR + prListLen;

    /* Parse the Naming Authority. */
    uint16_t namingAuthLen;
    std::copy_n(buff.data() + pos, slp::request::SIZE_NAMING,
                (uint8_t*)&namingAuthLen);

    pos += slp::request::SIZE_NAMING;

    namingAuthLen = endian::from_network(namingAuthLen);

    if (namingAuthLen == 0 || namingAuthLen == 0xffff)
    {
        req.body.srvtyperqst.namingAuth = "";
    }
    else
    {
        req.body.srvtyperqst.namingAuth.insert(
            0, (const char*)buff.data() + pos, namingAuthLen);
    }

    pos += namingAuthLen;

    /* Parse the <scope-list>. */
    uint16_t scopeListLen;
    std::copy_n(buff.data() + pos, slp::request::SIZE_SCOPE,
                (uint8_t*)&scopeListLen);

    pos += slp::request::SIZE_SCOPE;

    scopeListLen = endian::from_network(scopeListLen);

    req.body.srvtyperqst.scopeList.insert(0, (const char*)buff.data() + pos,
                                          scopeListLen);

    return slp::SUCCESS;
}

int parseSrvRqst(const buffer& buff, Message& req)
{
    /*  0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |      length of <PRList>       |        <PRList> String        \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |   length of <service-type>    |    <service-type> String      \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |    length of <scope-list>     |     <scope-list> String       \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |  length of predicate string   |  Service Request <predicate>  \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |  length of <SLP SPI> string   |       <SLP SPI> String        \
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

    /* Enforce v2 service request size limits. */
    if (buff.size() < slp::request::MIN_SRV_LEN)
    {
        return (int)slp::Error::PARSE_ERROR;
    }

    /* 1) Parse the PRList. */
    uint16_t prListLen;
    std::copy_n(buff.data() + slp::request::OFFSET_PR_LEN,
                slp::request::SIZE_PRLIST, (uint8_t*)&prListLen);

    auto pos = slp::request::OFFSET_PR_LEN + slp::request::SIZE_PRLIST;

    prListLen = endian::from_network(prListLen);

    req.body.srvrqst.prList.insert(0, (const char*)buff.data() + pos,
                                   prListLen);

    pos += prListLen;

    /* 2) Parse the <service-type> string. */
    uint16_t srvTypeLen;
    std::copy_n(buff.data() + pos, slp::request::SIZE_SERVICE_TYPE,
                (uint8_t*)&srvTypeLen);

    srvTypeLen = endian::from_network(srvTypeLen);

    pos += slp::request::SIZE_SERVICE_TYPE;

    req.body.srvrqst.srvType.insert(0, (const char*)buff.data() + pos,
                                    srvTypeLen);

    pos += srvTypeLen;

    /* 3) Parse the <scope-list> string. */
    uint16_t scopeListLen;
    std::copy_n(buff.data() + pos, slp::request::SIZE_SCOPE,
                (uint8_t*)&scopeListLen);

    scopeListLen = endian::from_network(scopeListLen);

    pos += slp::request::SIZE_SCOPE;

    req.body.srvrqst.scopeList.insert(0, (const char*)buff.data() + pos,
                                      scopeListLen);

    pos += scopeListLen;

    /* 4) Parse the <predicate> string. */
    uint16_t predicateLen;
    std::copy_n(buff.data() + pos, slp::request::SIZE_PREDICATE,
                (uint8_t*)&predicateLen);

    predicateLen = endian::from_network(predicateLen);
    pos += slp::request::SIZE_PREDICATE;

    req.body.srvrqst.predicate.insert(0, (const char*)buff.data() + pos,
                                      predicateLen);

    pos += predicateLen;

    /* 5) Parse the <SLP SPI> string. */
    uint16_t spistrLen;
    std::copy_n(buff.data() + pos, slp::request::SIZE_SLPI,
                (uint8_t*)&spistrLen);

    spistrLen = endian::from_network(spistrLen);
    pos += slp::request::SIZE_SLPI;

    req.body.srvrqst.spistr.insert(0, (const char*)buff.data() + pos,
                                   spistrLen);

    return slp::SUCCESS;
}
} // namespace internal

std::tuple<int, Message> parseBuffer(const buffer& buff)
{
    Message req;
    int rc = slp::SUCCESS;
    /* parse the header first */
    std::tie(rc, req) = internal::parseHeader(buff);
    if (!rc)
    {
        /* switch on the function id to parse the body */
        switch (req.header.functionID)
        {
            case (uint8_t)slp::FunctionType::SRVTYPERQST:
                rc = internal::parseSrvTypeRqst(buff, req);
                break;
            case (uint8_t)slp::FunctionType::SRVRQST:
                rc = internal::parseSrvRqst(buff, req);
                break;
            default:
                rc = (int)slp::Error::MSG_NOT_SUPPORTED;
        }
    }
    return std::make_tuple(rc, std::move(req));
}
} // namespace parser
} // namespace slp
