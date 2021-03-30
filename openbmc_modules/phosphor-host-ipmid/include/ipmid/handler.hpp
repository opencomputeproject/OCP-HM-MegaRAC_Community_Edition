/**
 * Copyright © 2018 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <algorithm>
#include <boost/asio/spawn.hpp>
#include <boost/callable_traits.hpp>
#include <cstdint>
#include <exception>
#include <ipmid/api-types.hpp>
#include <ipmid/message.hpp>
#include <memory>
#include <optional>
#include <phosphor-logging/log.hpp>
#include <tuple>
#include <user_channel/channel_layer.hpp>
#include <utility>

#ifdef ALLOW_DEPRECATED_API
#include <ipmid/api.h>

#include <ipmid/oemrouter.hpp>
#endif /* ALLOW_DEPRECATED_API */

namespace ipmi
{

template <typename... Args>
static inline message::Response::ptr
    errorResponse(message::Request::ptr request, ipmi::Cc cc, Args&&... args)
{
    message::Response::ptr response = request->makeResponse();
    response->cc = cc;
    response->pack(args...);
    return response;
}
static inline message::Response::ptr
    errorResponse(message::Request::ptr request, ipmi::Cc cc)
{
    message::Response::ptr response = request->makeResponse();
    response->cc = cc;
    return response;
}

/**
 * @brief Handler base class for dealing with IPMI request/response
 *
 * The subclasses are all templated so they can provide access to any type
 * of command callback functions.
 */
class HandlerBase
{
  public:
    using ptr = std::shared_ptr<HandlerBase>;

    /** @brief wrap the call to the registered handler with the request
     *
     * This is called from the running queue context after it has already
     * created a request object that contains all the information required to
     * execute the ipmi command. This function will return the response object
     * pointer that owns the response object that will ultimately get sent back
     * to the requester.
     *
     * This is a non-virtual function wrapper to the virtualized executeCallback
     * function that actually does the work. This is required because of how
     * templates and virtualization work together.
     *
     * @param request a shared_ptr to a Request object
     *
     * @return a shared_ptr to a Response object
     */
    message::Response::ptr call(message::Request::ptr request)
    {
        return executeCallback(request);
    }

  private:
    /** @brief call the registered handler with the request
     *
     * This is called from the running queue context after it has already
     * created a request object that contains all the information required to
     * execute the ipmi command. This function will return the response object
     * pointer that owns the response object that will ultimately get sent back
     * to the requester.
     *
     * @param request a shared_ptr to a Request object
     *
     * @return a shared_ptr to a Response object
     */
    virtual message::Response::ptr
        executeCallback(message::Request::ptr request) = 0;
};

/**
 * @brief Main IPMI handler class
 *
 * New IPMI handlers will resolve into this class, which will read the signature
 * of the registering function, attempt to extract the appropriate arguments
 * from a request, pass the arguments to the function, and then pack the
 * response of the function back into an IPMI response.
 */
template <typename Handler>
class IpmiHandler final : public HandlerBase
{
  public:
    explicit IpmiHandler(Handler&& handler) :
        handler_(std::forward<Handler>(handler))
    {
    }

  private:
    Handler handler_;

    /** @brief call the registered handler with the request
     *
     * This is called from the running queue context after it has already
     * created a request object that contains all the information required to
     * execute the ipmi command. This function will return the response object
     * pointer that owns the response object that will ultimately get sent back
     * to the requester.
     *
     * Because this is the new variety of IPMI handler, this is the function
     * that attempts to extract the requested parameters in order to pass them
     * onto the callback function and then packages up the response into a plain
     * old vector to pass back to the caller.
     *
     * @param request a shared_ptr to a Request object
     *
     * @return a shared_ptr to a Response object
     */
    message::Response::ptr
        executeCallback(message::Request::ptr request) override
    {
        message::Response::ptr response = request->makeResponse();

        using CallbackSig = boost::callable_traits::args_t<Handler>;
        using InputArgsType = typename utility::DecayTuple<CallbackSig>::type;
        using UnpackArgsType = typename utility::StripFirstArgs<
            utility::NonIpmiArgsCount<InputArgsType>::size(),
            InputArgsType>::type;
        using ResultType = boost::callable_traits::return_type_t<Handler>;

        UnpackArgsType unpackArgs;
        request->payload.trailingOk = false;
        ipmi::Cc unpackError = request->unpack(unpackArgs);
        if (unpackError != ipmi::ccSuccess)
        {
            response->cc = unpackError;
            return response;
        }
        /* callbacks can contain an optional first argument of one of:
         * 1) boost::asio::yield_context
         * 2) ipmi::Context::ptr
         * 3) ipmi::message::Request::ptr
         *
         * If any of those is part of the callback signature as the first
         * argument, it will automatically get packed into the parameter pack
         * here.
         *
         * One more special optional argument is an ipmi::message::Payload.
         * This argument can be in any position, though logically it makes the
         * most sense if it is the last. If this class is included in the
         * handler signature, it will allow for the handler to unpack optional
         * parameters. For example, the Set LAN Configuration Parameters
         * command takes variable length (and type) values for each of the LAN
         * parameters. This means that the  only fixed data is the channel and
         * parameter selector. All the remaining data can be extracted using
         * the Payload class and the unpack API available to the Payload class.
         */
        ResultType result;
        try
        {
            std::optional<InputArgsType> inputArgs;
            if constexpr (std::tuple_size<InputArgsType>::value > 0)
            {
                if constexpr (std::is_same<
                                  std::tuple_element_t<0, InputArgsType>,
                                  boost::asio::yield_context>::value)
                {
                    inputArgs.emplace(std::tuple_cat(
                        std::forward_as_tuple(request->ctx->yield),
                        std::move(unpackArgs)));
                }
                else if constexpr (std::is_same<
                                       std::tuple_element_t<0, InputArgsType>,
                                       ipmi::Context::ptr>::value)
                {
                    inputArgs.emplace(
                        std::tuple_cat(std::forward_as_tuple(request->ctx),
                                       std::move(unpackArgs)));
                }
                else if constexpr (std::is_same<
                                       std::tuple_element_t<0, InputArgsType>,
                                       ipmi::message::Request::ptr>::value)
                {
                    inputArgs.emplace(std::tuple_cat(
                        std::forward_as_tuple(request), std::move(unpackArgs)));
                }
                else
                {
                    // no special parameters were requested (but others were)
                    inputArgs.emplace(std::move(unpackArgs));
                }
            }
            else
            {
                // no parameters were requested
                inputArgs = std::move(unpackArgs);
            }

            // execute the registered callback function and get the
            // ipmi::RspType<>
            result = std::apply(handler_, *inputArgs);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Handler failed to catch exception",
                phosphor::logging::entry("EXCEPTION=%s", e.what()),
                phosphor::logging::entry("NETFN=%x", request->ctx->netFn),
                phosphor::logging::entry("CMD=%x", request->ctx->cmd));
            return errorResponse(request, ccUnspecifiedError);
        }
        catch (...)
        {
            std::exception_ptr eptr;
            try
            {
                eptr = std::current_exception();
                if (eptr)
                {
                    std::rethrow_exception(eptr);
                }
            }
            catch (const std::exception& e)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Handler failed to catch exception",
                    phosphor::logging::entry("EXCEPTION=%s", e.what()),
                    phosphor::logging::entry("NETFN=%x", request->ctx->netFn),
                    phosphor::logging::entry("CMD=%x", request->ctx->cmd));
                return errorResponse(request, ccUnspecifiedError);
            }
        }

        response->cc = std::get<0>(result);
        auto payload = std::get<1>(result);
        // check for optional payload
        if (payload)
        {
            response->pack(*payload);
        }
        return response;
    }
};

#ifdef ALLOW_DEPRECATED_API
static constexpr size_t maxLegacyBufferSize = 64 * 1024;
/**
 * @brief Legacy IPMI handler class
 *
 * Legacy IPMI handlers will resolve into this class, which will behave the same
 * way as the legacy IPMI queue, passing in a big buffer for the request and a
 * big buffer for the response.
 *
 * As soon as all the handlers have been rewritten, this class will be marked as
 * deprecated and eventually removed.
 */
template <>
class IpmiHandler<ipmid_callback_t> final : public HandlerBase
{
  public:
    explicit IpmiHandler(const ipmid_callback_t& handler, void* ctx = nullptr) :
        handler_(handler), handlerCtx(ctx)
    {
    }

  private:
    ipmid_callback_t handler_;
    void* handlerCtx;

    /** @brief call the registered handler with the request
     *
     * This is called from the running queue context after it has already
     * created a request object that contains all the information required to
     * execute the ipmi command. This function will return the response object
     * pointer that owns the response object that will ultimately get sent back
     * to the requester.
     *
     * Because this is the legacy variety of IPMI handler, this function does
     * not really have to do much other than pass the payload to the callback
     * and return response to the caller.
     *
     * @param request a shared_ptr to a Request object
     *
     * @return a shared_ptr to a Response object
     */
    message::Response::ptr
        executeCallback(message::Request::ptr request) override
    {
        message::Response::ptr response = request->makeResponse();
        // allocate a big response buffer here
        response->payload.resize(maxLegacyBufferSize);

        size_t len = request->payload.size() - request->payload.rawIndex;
        Cc ccRet{ccSuccess};
        try
        {
            ccRet =
                handler_(request->ctx->netFn, request->ctx->cmd,
                         request->payload.data() + request->payload.rawIndex,
                         response->payload.data(), &len, handlerCtx);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Legacy Handler failed to catch exception",
                phosphor::logging::entry("EXCEPTION=%s", e.what()),
                phosphor::logging::entry("NETFN=%x", request->ctx->netFn),
                phosphor::logging::entry("CMD=%x", request->ctx->cmd));
            return errorResponse(request, ccUnspecifiedError);
        }
        catch (...)
        {
            std::exception_ptr eptr;
            try
            {
                eptr = std::current_exception();
                if (eptr)
                {
                    std::rethrow_exception(eptr);
                }
            }
            catch (const std::exception& e)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Handler failed to catch exception",
                    phosphor::logging::entry("EXCEPTION=%s", e.what()),
                    phosphor::logging::entry("NETFN=%x", request->ctx->netFn),
                    phosphor::logging::entry("CMD=%x", request->ctx->cmd));
                return errorResponse(request, ccUnspecifiedError);
            }
        }
        response->cc = ccRet;
        response->payload.resize(len);
        return response;
    }
};

/**
 * @brief Legacy IPMI OEM handler class
 *
 * Legacy IPMI OEM handlers will resolve into this class, which will behave the
 * same way as the legacy IPMI queue, passing in a big buffer for the request
 * and a big buffer for the response.
 *
 * As soon as all the handlers have been rewritten, this class will be marked as
 * deprecated and eventually removed.
 */
template <>
class IpmiHandler<oem::Handler> final : public HandlerBase
{
  public:
    explicit IpmiHandler(const oem::Handler& handler) : handler_(handler)
    {
    }

  private:
    oem::Handler handler_;

    /** @brief call the registered handler with the request
     *
     * This is called from the running queue context after it has already
     * created a request object that contains all the information required to
     * execute the ipmi command. This function will return the response object
     * pointer that owns the response object that will ultimately get sent back
     * to the requester.
     *
     * Because this is the legacy variety of IPMI handler, this function does
     * not really have to do much other than pass the payload to the callback
     * and return response to the caller.
     *
     * @param request a shared_ptr to a Request object
     *
     * @return a shared_ptr to a Response object
     */
    message::Response::ptr
        executeCallback(message::Request::ptr request) override
    {
        message::Response::ptr response = request->makeResponse();
        // allocate a big response buffer here
        response->payload.resize(maxLegacyBufferSize);

        size_t len = request->payload.size() - request->payload.rawIndex;
        Cc ccRet{ccSuccess};
        try
        {
            ccRet =
                handler_(request->ctx->cmd,
                         request->payload.data() + request->payload.rawIndex,
                         response->payload.data(), &len);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Legacy OEM Handler failed to catch exception",
                phosphor::logging::entry("EXCEPTION=%s", e.what()),
                phosphor::logging::entry("NETFN=%x", request->ctx->netFn),
                phosphor::logging::entry("CMD=%x", request->ctx->cmd));
            return errorResponse(request, ccUnspecifiedError);
        }
        catch (...)
        {
            std::exception_ptr eptr;
            try
            {
                eptr = std::current_exception();
                if (eptr)
                {
                    std::rethrow_exception(eptr);
                }
            }
            catch (const std::exception& e)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Handler failed to catch exception",
                    phosphor::logging::entry("EXCEPTION=%s", e.what()),
                    phosphor::logging::entry("NETFN=%x", request->ctx->netFn),
                    phosphor::logging::entry("CMD=%x", request->ctx->cmd));
                return errorResponse(request, ccUnspecifiedError);
            }
        }
        response->cc = ccRet;
        response->payload.resize(len);
        return response;
    }
};

/**
 * @brief create a legacy IPMI handler class and return a shared_ptr
 *
 * The queue uses a map of pointers to do the lookup. This function returns the
 * shared_ptr that owns the Handler object.
 *
 * This is called internally via the ipmi_register_callback function.
 *
 * @param handler the function pointer to the callback
 *
 * @return A shared_ptr to the created handler object
 */
inline auto makeLegacyHandler(const ipmid_callback_t& handler,
                              void* ctx = nullptr)
{
    HandlerBase::ptr ptr(new IpmiHandler<ipmid_callback_t>(handler, ctx));
    return ptr;
}

/**
 * @brief create a legacy IPMI OEM handler class and return a shared_ptr
 *
 * The queue uses a map of pointers to do the lookup. This function returns the
 * shared_ptr that owns the Handler object.
 *
 * This is called internally via the Router::registerHandler method.
 *
 * @param handler the function pointer to the callback
 *
 * @return A shared_ptr to the created handler object
 */
inline auto makeLegacyHandler(oem::Handler&& handler)
{
    HandlerBase::ptr ptr(
        new IpmiHandler<oem::Handler>(std::forward<oem::Handler>(handler)));
    return ptr;
}
#endif // ALLOW_DEPRECATED_API

/**
 * @brief create an IPMI handler class and return a shared_ptr
 *
 * The queue uses a map of pointers to do the lookup. This function returns the
 * shared_ptr that owns the Handler object.
 *
 * This is called internally via the ipmi::registerHandler function.
 *
 * @param handler the function pointer to the callback
 *
 * @return A shared_ptr to the created handler object
 */
template <typename Handler>
inline auto makeHandler(Handler&& handler)
{
    HandlerBase::ptr ptr(
        new IpmiHandler<Handler>(std::forward<Handler>(handler)));
    return ptr;
}

namespace impl
{

// IPMI command handler registration implementation
bool registerHandler(int prio, NetFn netFn, Cmd cmd, Privilege priv,
                     ::ipmi::HandlerBase::ptr handler);
bool registerGroupHandler(int prio, Group group, Cmd cmd, Privilege priv,
                          ::ipmi::HandlerBase::ptr handler);
bool registerOemHandler(int prio, Iana iana, Cmd cmd, Privilege priv,
                        ::ipmi::HandlerBase::ptr handler);

} // namespace impl

/**
 * @brief main IPMI handler registration function
 *
 * This function should be used to register all new-style IPMI handler
 * functions. This function just passes the callback to makeHandler, which
 * creates a new wrapper object that will automatically extract the appropriate
 * parameters for the callback function as well as pack up the response.
 *
 * @param prio - priority at which to register; see api.hpp
 * @param netFn - the IPMI net function number to register
 * @param cmd - the IPMI command number to register
 * @param priv - the IPMI user privilige required for this command
 * @param handler - the callback function that will handle this request
 *
 * @return bool - success of registering the handler
 */
template <typename Handler>
bool registerHandler(int prio, NetFn netFn, Cmd cmd, Privilege priv,
                     Handler&& handler)
{
    auto h = ipmi::makeHandler(std::forward<Handler>(handler));
    return impl::registerHandler(prio, netFn, cmd, priv, h);
}

/**
 * @brief register a IPMI OEM group handler
 *
 * From IPMI 2.0 spec Network Function Codes Table (Row 2Ch):
 * The first data byte position in requests and responses under this network
 * function identifies the defining body that specifies command functionality.
 * Software assumes that the command and completion code field positions will
 * hold command and completion code values.
 *
 * The following values are used to identify the defining body:
 * 00h PICMG - PCI Industrial Computer Manufacturer’s Group.  (www.picmg.com)
 * 01h DMTF Pre-OS Working Group ASF Specification (www.dmtf.org)
 * 02h Server System Infrastructure (SSI) Forum (www.ssiforum.org)
 * 03h VITA Standards Organization (VSO) (www.vita.com)
 * DCh DCMI Specifications (www.intel.com/go/dcmi)
 * all other Reserved
 *
 * When this network function is used, the ID for the defining body occupies
 * the first data byte in a request, and the second data byte (following the
 * completion code) in a response.
 *
 * @tparam Handler - implicitly specified callback function type
 * @param prio - priority at which to register; see api.hpp
 * @param netFn - the IPMI net function number to register
 * @param cmd - the IPMI command number to register
 * @param priv - the IPMI user privilige required for this command
 * @param handler - the callback function that will handle this request
 *
 * @return bool - success of registering the handler
 *
 */
template <typename Handler>
void registerGroupHandler(int prio, Group group, Cmd cmd, Privilege priv,
                          Handler&& handler)
{
    auto h = ipmi::makeHandler(handler);
    impl::registerGroupHandler(prio, group, cmd, priv, h);
}

/**
 * @brief register a IPMI OEM IANA handler
 *
 * From IPMI spec Network Function Codes Table (Row 2Eh):
 * The first three data bytes of requests and responses under this network
 * function explicitly identify the OEM or non-IPMI group that specifies the
 * command functionality. While the OEM or non-IPMI group defines the
 * functional semantics for the cmd and remaining data fields, the cmd field
 * is required to hold the same value in requests and responses for a given
 * operation in order to be supported under the IPMI message handling and
 * transport mechanisms.
 *
 * When this network function is used, the IANA Enterprise Number for the
 * defining body occupies the first three data bytes in a request, and the
 * first three data bytes following the completion code position in a
 * response.
 *
 * @tparam Handler - implicitly specified callback function type
 * @param prio - priority at which to register; see api.hpp
 * @param netFn - the IPMI net function number to register
 * @param cmd - the IPMI command number to register
 * @param priv - the IPMI user privilige required for this command
 * @param handler - the callback function that will handle this request
 *
 * @return bool - success of registering the handler
 *
 */
template <typename Handler>
void registerOemHandler(int prio, Iana iana, Cmd cmd, Privilege priv,
                        Handler&& handler)
{
    auto h = ipmi::makeHandler(handler);
    impl::registerOemHandler(prio, iana, cmd, priv, h);
}

} // namespace ipmi

#ifdef ALLOW_DEPRECATED_API
/**
 * @brief legacy IPMI handler registration function
 *
 * This function should be used to register all legacy IPMI handler
 * functions. This function just behaves just as the legacy registration
 * mechanism did, silently replacing any existing handler with a new one.
 *
 * @param netFn - the IPMI net function number to register
 * @param cmd - the IPMI command number to register
 * @param context - ignored
 * @param handler - the callback function that will handle this request
 * @param priv - the IPMI user privilige required for this command
 */
// [[deprecated("Use ipmi::registerHandler() instead")]]
void ipmi_register_callback(ipmi_netfn_t netFn, ipmi_cmd_t cmd,
                            ipmi_context_t context, ipmid_callback_t handler,
                            ipmi_cmd_privilege_t priv);

#endif /* ALLOW_DEPRECATED_API */
