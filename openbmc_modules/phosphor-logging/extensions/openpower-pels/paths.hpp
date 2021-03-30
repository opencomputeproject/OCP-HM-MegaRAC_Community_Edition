#pragma once
#include <filesystem>

namespace openpower
{
namespace pels
{

/**
 * @brief Returns the path to the PEL ID file
 */
std::filesystem::path getPELIDFile();

/**
 * @brief Returns the path to the PEL repository
 */
std::filesystem::path getPELRepoPath();

/**
 * @brief Returns the path to the read only data directory
 */
std::filesystem::path getPELReadOnlyDataPath();

/**
 * @brief Returns the maximum size in bytes allocated to store PELs.
 *
 * This is still in paths.c/hpp even though it doesn't return a path
 * because this file is easy to override when testing.
 *
 * @return size_t The maximum size in bytes to use for storing PELs.
 */
size_t getPELRepoSize();

/**
 * @brief Returns the maximum number of PELs allowed
 *
 * This is still in paths.c/hpp even though it doesn't return a path
 * because this file is easy to override when testing.
 *
 * @return size_t The maximum number of PELs to allow in the repository.
 */
size_t getMaxNumPELs();

} // namespace pels
} // namespace openpower
