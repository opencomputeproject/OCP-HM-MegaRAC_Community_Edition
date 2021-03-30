#pragma once

#include "section.hpp"
#include "stream.hpp"

namespace openpower
{
namespace pels
{
namespace section_factory
{

/**
 * @brief Create a PEL section based on its data
 *
 * This creates the appropriate PEL section object based on the section ID in
 * the first 2 bytes of the stream, but returns the base class Section pointer.
 *
 * If there isn't a class specifically for that section, it defaults to
 * creating an instance of the 'Generic' class.
 *
 * @param[in] pelData - The PEL data stream
 *
 * @return std::unique_ptr<Section> - class of the appropriate type
 */
std::unique_ptr<Section> create(Stream& pelData);

} // namespace section_factory
} // namespace pels
} // namespace openpower
