#ifndef __IPMI_FRU_AREA_H__
#define __IPMI_FRU_AREA_H__

#include "frup.hpp"
#include "writefrudata.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using std::uint8_t;

/**
 * IPMIFruArea represents a piece of a FRU that is accessible over IPMI.
 */
class IPMIFruArea
{
  public:
    IPMIFruArea() = delete;
    ~IPMIFruArea() = default;

    /**
     * Construct an IPMIFruArea.
     *
     * @param[in] fruID - FRU identifier value
     * @param[in] type - the type of FRU area.
     * @param[in] bmcOnlyFru - Is this FRU only accessible via the BMC
     */
    IPMIFruArea(const uint8_t fruID, const ipmi_fru_area_type type,
                bool bmcOnlyFru = false);

    /**
     * Set whether the FRU is present.
     *
     * @param[in] present - True if present.
     */
    inline void setPresent(const bool present)
    {
        isPresent = present;
    }

    /**
     * Retrieves the FRU's ID.
     *
     * @return the FRU ID.
     */
    uint8_t getFruID() const
    {
        return fruID;
    }

    /**
     *  Returns the length of the FRU data.
     *
     * @return the number of bytes.
     */
    size_t getLength() const
    {
        return data.size();
    }

    /**
     * Returns the type of the current FRU area.
     *
     * @return the type of FRU area
     */
    ipmi_fru_area_type getType() const
    {
        return type;
    }

    /**
     * Returns the FRU area name.
     *
     * @return the FRU area name
     */
    const char* getName() const
    {
        return name.c_str();
    }

    /**
     * Returns the data portion.
     *
     * @return pointer to data
     */
    inline const uint8_t* getData() const
    {
        return data.data();
    }

    /**
     * Accepts a pointer to data and sets it in the object.
     *
     * @param[in] value - The data to copy into the FRU area
     * @param[in] length - the number of bytes value points to
     */
    void setData(const uint8_t* value, const size_t length);

  private:
    // Unique way of identifying a FRU
    uint8_t fruID = 0;

    // Type of the FRU matching offsets in common header
    ipmi_fru_area_type type = IPMI_FRU_AREA_INTERNAL_USE;

    // Name of the FRU area. ( BOARD/CHASSIS/PRODUCT )
    std::string name;

    // Special bit for BMC readable eeprom only.
    bool bmcOnlyFru = false;

    // If a FRU is physically present.
    bool isPresent = false;

    // Whether a particular area is valid ?
    bool isValid = false;

    // Actual area data.
    std::vector<uint8_t> data;
};

#endif
