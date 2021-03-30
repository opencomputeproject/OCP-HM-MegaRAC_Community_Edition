#pragma once

#include "libpldm/bios_table.h"

#include <cstring>
#include <memory>
#include <type_traits>
#include <vector>

namespace pldm
{
namespace bios
{
namespace utils
{

using Table = std::vector<uint8_t>;

/** @class BIOSTableIter
 *  @brief Const Iterator of a BIOS Table
 */
template <pldm_bios_table_types tableType>
class BIOSTableIter
{
  public:
    /** @struct EndSentinel
     *  @brief Auxiliary struct to delimit a range
     */
    struct EndSentinel
    {};

    /** @struct iterator
     *  @brief iterator owns the BIOS table
     */
    class iterator
    {
      public:
        /** @brief Get entry type by specifying \p tableType
         */
        using T = typename std::conditional<
            tableType == PLDM_BIOS_STRING_TABLE, pldm_bios_string_table_entry,
            typename std::conditional<
                tableType == PLDM_BIOS_ATTR_TABLE, pldm_bios_attr_table_entry,
                typename std::conditional<tableType == PLDM_BIOS_ATTR_VAL_TABLE,
                                          pldm_bios_attr_val_table_entry,
                                          void>::type>::type>::type;
        static_assert(!std::is_void<T>::value);

        /** @brief Constructors iterator
         *
         *  @param[in] data - Pointer to a table
         *  @param[in] length - The length of the table
         */
        explicit iterator(const void* data, size_t length) noexcept :
            iter(pldm_bios_table_iter_create(data, length, tableType),
                 pldm_bios_table_iter_free)
        {}

        /** @brief Get the entry pointed by the iterator
         *
         *  @return Poiner to the entry
         */
        const T* operator*() const
        {
            return reinterpret_cast<const T*>(
                pldm_bios_table_iter_value(iter.get()));
        }

        /** @brief Make the iterator point to the next entry
         *
         *  @return The iterator itself
         */
        iterator& operator++()
        {
            pldm_bios_table_iter_next(iter.get());
            return *this;
        }

        /** @brief Check if the iterator ends
         *
         *  @return True if the iterator ends
         */
        bool operator==(const EndSentinel&) const
        {
            return pldm_bios_table_iter_is_end(iter.get());
        }

        /** @brief Check if the iterator ends
         *
         *  @return False if the iterator ends
         */
        bool operator!=(const EndSentinel& endSentinel) const
        {
            return !operator==(endSentinel);
        }

      private:
        std::unique_ptr<pldm_bios_table_iter,
                        decltype(&pldm_bios_table_iter_free)>
            iter;
    };

    /** @brief Constructors BIOSTableIterator
     *
     *  @param[in] data - Pointer to a table
     *  @param[in] length - The length of the table
     */
    BIOSTableIter(const void* data, size_t length) noexcept :
        tableData(data), tableSize(length)
    {}

    /** @brief Get the iterator to the beginning
     *
     *  @return An iterator to the beginning
     */
    iterator begin()
    {
        return iterator(tableData, tableSize);
    }

    /** @brief Get the iterator to the end
     *
     *  @return An iterator to the end
     */
    EndSentinel end()
    {
        return {};
    }

  private:
    const void* tableData;
    size_t tableSize;
};

} // namespace utils
} // namespace bios
} // namespace pldm
