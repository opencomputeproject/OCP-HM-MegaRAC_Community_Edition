#pragma once

#include "callout.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{
namespace src
{

constexpr uint8_t calloutsSubsectionID = 0xC0;
constexpr size_t maxNumberOfCallouts = 10;

/**
 * @class Callouts
 *
 * This is an optional subsection of the SRC section in a PEL
 * that holds callouts (represented as Callout objects).
 * It is at the end of the SRC section, and there can only be one
 * of these present in the SRC.
 *
 * If an SRC doesn't have any callouts, this object won't be created.
 */
class Callouts
{
  public:
    ~Callouts() = default;
    Callouts(const Callouts&) = delete;
    Callouts& operator=(const Callouts&) = delete;
    Callouts(Callouts&&) = delete;
    Callouts& operator=(Callouts&&) = delete;

    /**
     * @brief Constructor
     *
     * Creates the object with no callouts.
     */
    Callouts() :
        _subsectionID(calloutsSubsectionID), _subsectionFlags(0),
        _subsectionWordLength(1)
    {
    }

    /**
     * @brief Constructor
     *
     * Fills in this class's data fields from the stream.
     *
     * @param[in] pel - the PEL data stream
     */
    explicit Callouts(Stream& pel);

    /**
     * @brief Flatten the object into the stream
     *
     * @param[in] stream - The stream to write to
     */
    void flatten(Stream& pel) const;

    /**
     * @brief Returns the size of this object when flattened into a PEL
     *
     * @return size_t - The size of the section
     */
    size_t flattenedSize()
    {
        return _subsectionWordLength * 4;
    }

    /**
     * @brief Returns the contained callouts
     *
     * @return const std::vector<std::unique_ptr<Callout>>&
     */
    const std::vector<std::unique_ptr<Callout>>& callouts() const
    {
        return _callouts;
    }

    /**
     * @brief Adds a callout
     *
     * @param[in] callout - The callout to add
     */
    void addCallout(std::unique_ptr<Callout> callout);

  private:
    /**
     * @brief The ID of this subsection, which is 0xC0.
     */
    uint8_t _subsectionID;

    /**
     * @brief Subsection flags.  Always 0.
     */
    uint8_t _subsectionFlags;

    /**
     * @brief Subsection length in 4B words.
     *
     * (Subsection is always a multiple of 4B)
     */
    uint16_t _subsectionWordLength;

    /**
     * @brief The contained callouts
     */
    std::vector<std::unique_ptr<Callout>> _callouts;
};

} // namespace src

} // namespace pels
} // namespace openpower
