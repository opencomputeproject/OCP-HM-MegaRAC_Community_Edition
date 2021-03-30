#pragma once

#include <sdbusplus/bus.hpp>

#include <string>

namespace phosphor
{
namespace software
{
namespace updater
{

class Helper
{
  public:
    Helper() = delete;
    Helper(const Helper&) = delete;
    Helper& operator=(const Helper&) = delete;
    Helper(Helper&&) = default;
    Helper& operator=(Helper&&) = default;

    /** @brief Constructor
     *
     *  @param[in] bus - sdbusplus D-Bus bus connection
     */
    Helper(sdbusplus::bus::bus& bus) : bus(bus)
    {
        // Empty
    }

    /** @brief Set an environment variable to the specified value
     *
     * @param[in] entryId - The variable name
     * @param[in] value - The variable value
     */
    void setEntry(const std::string& entryId, uint8_t value);

    /** @brief Clear an image with the entry id
     *
     * @param[in] entryId - The image entry id
     */
    void clearEntry(const std::string& entryId);

    /** @brief Clean up all the unused images */
    void cleanup();

    /** @brief Do factory reset */
    void factoryReset();

    /** @brief Remove the image with the version id
     *
     * @param[in] versionId - The version id of the image
     */
    void removeVersion(const std::string& versionId);

    /** @brief Update version id in uboot env
     *
     * @param[in] versionId - The version id of the image
     */
    void updateUbootVersionId(const std::string& versionId);

    /** @brief Mirror Uboot to the alt uboot partition */
    void mirrorAlt();

  private:
    /** @brief Persistent sdbusplus D-Bus bus connection. */
    sdbusplus::bus::bus& bus;
};

} // namespace updater
} // namespace software
} // namespace phosphor
