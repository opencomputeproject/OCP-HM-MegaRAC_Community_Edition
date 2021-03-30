#ifndef UTILS_H__
#define UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "pldm_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @struct variable_field
 *
 *  Structure representing variable filed in the pldm message
 */
struct variable_field {
	const uint8_t *ptr;
	size_t length;
};

/** @brief Compute Crc8(same as the one used by SMBUS)
 *
 *  @param[in] data - Pointer to the target data
 *  @param[in] size - Size of the data
 *  @return The checksum
 */
uint8_t crc8(const void *data, size_t size);

/** @brief Compute Crc32(same as the one used by IEEE802.3)
 *
 *  @param[in] data - Pointer to the target data
 *  @param[in] size - Size of the data
 *  @return The checksum
 */
uint32_t crc32(const void *data, size_t size);

/** @brief Convert ver32_t to string
 *  @param[in] version - Pointer to ver32_t
 *  @param[out] buffer - Pointer to the buffer
 *  @param[in] buffer_size - Size of the buffer
 *  @return The number of characters(excluding the null byte) or negative if
 * error is encountered
 */
int ver2str(const ver32_t *version, char *buffer, size_t buffer_size);

/** @brief Convert bcd number(uint8_t) to decimal
 *  @param[in] bcd - bcd number
 *  @return the decimal number
 */
uint8_t bcd2dec8(uint8_t bcd);

/** @brief Convert decimal number(uint8_t) to bcd
 *  @param[in] dec - decimal number
 *  @return the bcd number
 */
uint8_t dec2bcd8(uint8_t dec);

/** @brief Convert bcd number(uint16_t) to decimal
 *  @param[in] bcd - bcd number
 *  @return the decimal number
 */
uint16_t bcd2dec16(uint16_t bcd);

/** @brief Convert decimal number(uint16_t) to bcd
 *  @param[in] dec - decimal number
 *  @return the bcd number
 */
uint16_t dec2bcd16(uint16_t dec);

/** @brief Convert bcd number(uint32_t) to decimal
 *  @param[in] bcd - bcd number
 *  @return the decimal number
 */
uint32_t bcd2dec32(uint32_t bcd);

/** @brief Convert decimal number(uint32_t) to bcd
 *  @param[in] dec - decimal number
 *  @return the bcd number
 */
uint32_t dec2bcd32(uint32_t dec);

/** @brief Check whether the input time is legal
 *
 *  @param[in] seconds. Value range 0~59
 *  @param[in] minutes. Value range 0~59
 *  @param[in] hours. Value range 0~23
 *  @param[in] day. Value range 1~31
 *  @param[in] month. Value range 1~12
 *  @param[in] year. Value range 1970~
 *  @return true if time is legal,false if time is illegal
 */
bool is_time_legal(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day,
		   uint8_t month, uint16_t year);

#ifdef __cplusplus
}
#endif

#endif
