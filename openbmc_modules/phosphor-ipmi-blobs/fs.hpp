#pragma once

#include <functional>
#include <string>
#include <vector>

namespace blobs
{
using PathMatcher = std::function<bool(const std::string& filename)>;

/**
 * Returns a list of library paths.  Checks against match method.
 *
 * TODO: Can be dropped if we implement a clean fs wrapper for test injection.
 *
 * @param[in] path - the path to search
 * @param[in] check - the function to call to check the path
 * @return a list of paths that match the criteria
 */
std::vector<std::string> getLibraryList(const std::string& path,
                                        PathMatcher check);

} // namespace blobs
