#pragma once

#include "config.h"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <string>

namespace phosphor
{
namespace led
{
namespace fru
{
namespace fault
{
namespace monitor
{

/** @brief Assert or deassert an LED based on the input FRU
 *  @param[in] bus       -  The Dbus bus object
 *  @param[in] path      -  Inventory path of the FRU
 *  @param[in] assert    -  Assert if true deassert if false
 */
void action(sdbusplus::bus::bus& bus, const std::string& path, bool assert);

class Remove;

/** @class Add
 *  @brief Implementation of LED handling during FRU fault
 *  @details This implements methods for watching for a FRU fault
 *  being logged to assert the corresponding LED
 */
class Add
{
  public:
    Add() = delete;
    ~Add() = default;
    Add(const Add&) = delete;
    Add& operator=(const Add&) = delete;
    Add(Add&&) = default;
    Add& operator=(Add&&) = default;

    /** @brief constructs Add a watch for FRU faults.
     *  @param[in] bus -  The Dbus bus object
     */
    Add(sdbusplus::bus::bus& bus) :
        matchCreated(
            bus,
            sdbusplus::bus::match::rules::interfacesAdded() +
                sdbusplus::bus::match::rules::path_namespace(
                    "/xyz/openbmc_project/logging"),
            std::bind(std::mem_fn(&Add::created), this, std::placeholders::_1))
    {
        processExistingCallouts(bus);
    }

  private:
    /** @brief sdbusplus signal match for fault created */
    sdbusplus::bus::match_t matchCreated;

    std::vector<std::unique_ptr<Remove>> removeWatches;

    /** @brief Callback function for fru fault created
     *  @param[in] msg       - Data associated with subscribed signal
     */
    void created(sdbusplus::message::message& msg);

    /** @brief This function process all callouts at application start
     *  @param[in] bus - The Dbus bus object
     */
    void processExistingCallouts(sdbusplus::bus::bus& bus);
};

/** @class Remove
 *  @brief Implementation of LED handling after resolution of FRU fault
 *  @details Implement methods for watching the resolution of FRU faults
 *  and deasserting corresponding LED.
 */
class Remove
{
  public:
    Remove() = delete;
    ~Remove() = default;
    Remove(const Remove&) = delete;
    Remove& operator=(const Remove&) = delete;
    Remove(Remove&&) = default;
    Remove& operator=(Remove&&) = default;

    /** @brief constructs Remove
     *  @param[in] bus  -  The Dbus bus object
     *  @param[in] path -  Inventory path to fru
     */
    Remove(sdbusplus::bus::bus& bus, const std::string& path) :
        inventoryPath(path),
        matchRemoved(bus, match(path),
                     std::bind(std::mem_fn(&Remove::removed), this,
                               std::placeholders::_1))
    {
        // Do nothing
    }

  private:
    /** @brief inventory path of the FRU */
    std::string inventoryPath;

    /** @brief sdbusplus signal matches for fault removed */
    sdbusplus::bus::match_t matchRemoved;

    /** @brief Callback function for fru fault created
     *  @param[in] msg       - Data associated with subscribed signal
     */
    void removed(sdbusplus::message::message& msg);

    /** @brief function to create fault remove match for a fru
     *  @param[in] path  - Inventory path of the faulty unit.
     */
    std::string match(const std::string& path)
    {
        namespace MatchRules = sdbusplus::bus::match::rules;

        std::string matchStmt =
            MatchRules::interfacesRemoved() +
            MatchRules::argNpath(0, path + "/" + CALLOUT_REV_ASSOCIATION);

        return matchStmt;
    }
};
} // namespace monitor
} // namespace fault
} // namespace fru
} // namespace led
} // namespace phosphor
