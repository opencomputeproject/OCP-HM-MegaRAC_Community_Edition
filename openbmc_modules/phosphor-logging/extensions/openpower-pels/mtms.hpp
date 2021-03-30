#pragma once

#include "stream.hpp"

#include <string>

namespace openpower
{
namespace pels
{

/** @class MTMS
 *
 * (M)achine (T)ype-(M)odel (S)erialNumber
 *
 * Represents the PEL's Machine Type / Model / Serial Number
 * structure, which shows up in multiple places in a PEL.
 *
 * It holds 8 bytes for the Type+Model, and 12 for the SN.
 * Unused bytes are set to 0s.
 *
 * The type and model are combined into a single field.
 */
class MTMS
{
  public:
    MTMS(const MTMS&) = default;
    MTMS& operator=(const MTMS&) = default;
    MTMS(MTMS&&) = default;
    MTMS& operator=(MTMS&&) = default;
    ~MTMS() = default;

    enum
    {
        mtmSize = 8,
        snSize = 12
    };

    /**
     * @brief Constructor
     */
    MTMS();

    /**
     * @brief Constructor
     *
     * @param[in] typeModel - The machine type+model
     * @param[in] serialNumber - The machine serial number
     */
    MTMS(const std::string& typeModel, const std::string& serialNumber);

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit MTMS(Stream& stream);

    /**
     * @brief Returns the raw machine type/model value
     *
     * @return std::array<uint8_t, mtmSize>&  - The TM value
     */
    const std::array<uint8_t, mtmSize>& machineTypeAndModelRaw() const
    {
        return _machineTypeAndModel;
    }

    /**
     * @brief Sets the machine type and model field
     *
     * @param[in] mtm - The new value
     */
    void setMachineTypeAndModel(const std::array<uint8_t, mtmSize>& mtm)
    {
        _machineTypeAndModel = mtm;
    }

    /**
     * @brief Returns the machine type/model value
     *
     * @return std::string - The TM value
     */
    std::string machineTypeAndModel() const
    {
        std::string mtm;

        // Get everything up to the 0s.
        for (size_t i = 0; (i < mtmSize) && (_machineTypeAndModel[i] != 0); i++)
        {
            mtm.push_back(_machineTypeAndModel[i]);
        }

        return mtm;
    }

    /**
     * @brief Returns the raw machine serial number value
     *
     * @return std::array<uint8_t, snSize>& - The SN value
     */
    const std::array<uint8_t, snSize>& machineSerialNumberRaw() const
    {
        return _serialNumber;
    }

    /**
     * @brief Sets the machine serial number field
     *
     * @param[in] sn - The new value
     */
    void setMachineSerialNumber(const std::array<uint8_t, snSize>& sn)
    {
        _serialNumber = sn;
    }

    /**
     * @brief Returns the machine serial number value
     *
     * @return std::string - The SN value
     */
    std::string machineSerialNumber() const
    {
        std::string sn;

        // Get everything up to the 0s.
        for (size_t i = 0; (i < snSize) && (_serialNumber[i] != 0); i++)
        {
            sn.push_back(_serialNumber[i]);
        }
        return sn;
    }

    /**
     * @brief Returns the size of the data when flattened
     *
     * @return size_t - The size of the data
     */
    static constexpr size_t flattenedSize()
    {
        return mtmSize + snSize;
    }

  private:
    /**
     * @brief The type+model value
     *
     *     Of the form TTTT-MMM where:
     *        TTTT = machine type
     *        MMM = machine model
     */
    std::array<uint8_t, mtmSize> _machineTypeAndModel;

    /**
     * @brief Machine Serial Number
     */
    std::array<uint8_t, snSize> _serialNumber;
};

/**
 * @brief Stream extraction operator for MTMS
 *
 * @param[in] s - the stream
 * @param[out] mtms - the MTMS object
 *
 * @return Stream&
 */
Stream& operator>>(Stream& s, MTMS& mtms);

/**
 * @brief Stream insertion operator for MTMS
 *
 * @param[out] s - the stream
 * @param[in] mtms - the MTMS object
 *
 * @return Stream&
 */
Stream& operator<<(Stream& s, const MTMS& mtms);

} // namespace pels
} // namespace openpower
