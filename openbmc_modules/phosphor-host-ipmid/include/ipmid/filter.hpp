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
#include <boost/callable_traits.hpp>
#include <cstdint>
#include <ipmid/api-types.hpp>
#include <ipmid/message.hpp>
#include <memory>
#include <tuple>
#include <utility>

namespace ipmi
{

using FilterFunction = ipmi::Cc(ipmi::message::Request::ptr);

/**
 * @brief Filter base class for dealing with IPMI request/response
 *
 * The subclasses are all templated so they can provide access to any type of
 * command callback functions.
 */
class FilterBase
{
  public:
    using ptr = std::shared_ptr<FilterBase>;

    virtual ipmi::Cc call(message::Request::ptr request) = 0;
};

/**
 * @brief filter concrete class
 *
 * This is the base template that ipmi filters will resolve into. This is
 * essentially just a wrapper to hold the filter callback so it can be stored in
 * the filter list.
 *
 * Filters are called with a ipmi::message::Request shared_ptr on all IPMI
 * commands in priority order and each filter has the opportunity to reject the
 * command (by returning an IPMI error competion code.) If all the filters
 * return success, the actual IPMI command will be executed. Filters can reject
 * the command for any reason, based on system state, the context, the command
 * payload, etc.
 */
template <typename Filter>
class IpmiFilter : public FilterBase
{
  public:
    IpmiFilter(Filter&& filter) : filter_(std::move(filter))
    {
    }

    ipmi::Cc call(message::Request::ptr request) override
    {
        return filter_(request);
    }

  private:
    Filter filter_;
};

/**
 * @brief helper function to construct a filter object
 *
 * This is called internally by the ipmi::registerFilter function.
 */
template <typename Filter>
static inline auto makeFilter(Filter&& filter)
{
    FilterBase::ptr ptr(new IpmiFilter<Filter>(std::forward<Filter>(filter)));
    return ptr;
}
template <typename Filter>
static inline auto makeFilter(const Filter& filter)
{
    Filter lFilter = filter;
    return makeFilter(std::forward<Filter>(lFilter));
}

namespace impl
{

// IPMI command filter registration implementation
void registerFilter(int prio, ::ipmi::FilterBase::ptr filter);

} // namespace impl

/**
 * @brief IPMI command filter registration function
 *
 * This function should be used to register IPMI command filter functions.
 * This function just passes the callback to makeFilter, which creates a
 * wrapper functor object that ultimately calls the callback.
 *
 * Filters are called with a ipmi::message::Request shared_ptr on all IPMI
 * commands in priority order and each filter has the opportunity to reject the
 * command (by returning an IPMI error competion code.) If all the filters
 * return success, the actual IPMI command will be executed. Filters can reject
 * the command for any reason, based on system state, the context, the command
 * payload, etc.
 *
 * @param prio - priority at which to register; see api.hpp
 * @param filter - the callback function that will handle this request
 *
 * @return bool - success of registering the handler
 */
template <typename Filter>
void registerFilter(int prio, Filter&& filter)
{
    auto f = ipmi::makeFilter(std::forward<Filter>(filter));
    impl::registerFilter(prio, f);
}

template <typename Filter>
void registerFilter(int prio, const Filter& filter)
{
    auto f = ipmi::makeFilter(filter);
    impl::registerFilter(prio, f);
}

} // namespace ipmi
