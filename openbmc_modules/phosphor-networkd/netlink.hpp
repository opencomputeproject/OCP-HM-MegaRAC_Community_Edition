#pragma once
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <functional>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace phosphor
{
namespace network
{
namespace netlink
{

/* @brief Called on each nlmsg received on the socket
 */
using ReceiveCallback = std::function<void(const nlmsghdr&, std::string_view)>;

namespace detail
{

void processMsg(std::string_view& msgs, bool& done, const ReceiveCallback& cb);

void performRequest(int protocol, void* data, size_t size,
                    const ReceiveCallback& cb);

} // namespace detail

/* @brief Call on a block of rtattrs to parse a single one out
 *        Updates the input to remove the attr parsed out.
 *
 * @param[in,out] attrs - The buffer holding rtattrs to parse
 * @return A tuple of rtattr header + data buffer for the attr
 */
std::tuple<rtattr, std::string_view> extractRtAttr(std::string_view& data);

/** @brief Performs a netlink request of the specified type with the given
 *  message Calls the callback upon receiving
 *
 *  @param[in] protocol - The netlink protocol to use when opening the socket
 *  @param[in] type     - The netlink message type
 *  @param[in] flags    - Additional netlink flags for the request
 *  @param[in] msg      - The message payload for the request
 *  @param[in] cb       - Called for each response message payload
 */
template <typename T>
void performRequest(int protocol, uint16_t type, uint16_t flags, const T& msg,
                    const ReceiveCallback& cb)
{
    static_assert(std::is_trivially_copyable_v<T>);

    struct
    {
        nlmsghdr hdr;
        T msg;
    } data{};
    data.hdr.nlmsg_len = sizeof(data);
    data.hdr.nlmsg_type = type;
    data.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK | flags;
    data.msg = msg;

    detail::performRequest(protocol, &data, sizeof(data), cb);
}

} // namespace netlink
} // namespace network
} // namespace phosphor
