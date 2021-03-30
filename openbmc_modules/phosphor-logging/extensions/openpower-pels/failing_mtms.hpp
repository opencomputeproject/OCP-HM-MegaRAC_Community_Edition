#pragma once

#include "data_interface.hpp"
#include "mtms.hpp"
#include "section.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{

/**
 * @class FailingMTMS
 *
 * This represents the Failing Enclosure MTMS section in a PEL.
 * It is a required section.  In the BMC case, the enclosure is
 * the system enclosure.
 */
class FailingMTMS : public Section
{
  public:
    FailingMTMS() = delete;
    ~FailingMTMS() = default;
    FailingMTMS(const FailingMTMS&) = default;
    FailingMTMS& operator=(const FailingMTMS&) = default;
    FailingMTMS(FailingMTMS&&) = default;
    FailingMTMS& operator=(FailingMTMS&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] dataIface - The object to use to obtain
     *                        the MTM and SN.
     */
    explicit FailingMTMS(const DataInterfaceBase& dataIface);

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit FailingMTMS(Stream& pel);

    /**
     * @brief Flatten the section into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& stream) const override;

    /**
     * @brief Returns the size of this section when flattened into a PEL
     *
     * @return size_t - the size of the section
     */
    static constexpr size_t flattenedSize()
    {
        return Section::flattenedSize() + MTMS::flattenedSize();
    }

    /**
     * @brief Returns the machine type+model as a string
     *
     * @return std::string the MTM
     */
    std::string getMachineTypeModel() const
    {
        return _mtms.machineTypeAndModel();
    }

    /**
     * @brief Returns the machine serial number as a string
     *
     * @return std::string the serial number
     */
    std::string getMachineSerialNumber() const
    {
        return _mtms.machineSerialNumber();
    }

    /**
     * @brief Get section in JSON.
     * @return std::optional<std::string> - Failing MTMS section in JSON
     */
    std::optional<std::string> getJSON() const override;

  private:
    /**
     * @brief Validates the section contents
     *
     * Updates _valid (in Section) with the results.
     */
    void validate() override;

    /**
     * @brief Fills in the object from the stream data
     *
     * @param[in] stream - The stream to read from
     */
    void unflatten(Stream& stream);

    /**
     * @brief The structure that holds the TM and SN fields.
     *        This is also used in other places in the PEL.
     */
    MTMS _mtms;
};

} // namespace pels

} // namespace openpower
