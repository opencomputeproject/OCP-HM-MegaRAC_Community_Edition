#pragma once

#include "internal/sys.hpp"
#include "manager.hpp"

#include <memory>
#include <string>

namespace blobs
{
using HandlerFactory = std::unique_ptr<GenericBlobInterface> (*)();

/**
 * The bitbake recipe symlinks the library lib*.so.? into the folder
 * only, and not the other names, .so, .so.?.?, .so.?.?.?
 *
 * Therefore only care if it's lib*.so.?
 *
 * @param[in] the path to check.
 * @return true if matches, false otherwise
 */
bool matchBlobHandler(const std::string& filename);

/**
 * @brief Given a path, find libraries (*.so only) and load them.
 *
 * @param[in] manager - pointer to a manager
 * @param[in] paths - list of fully qualified paths to libraries to load.
 * @param[in] sys - pointer to implementation of the dlsys interface.
 */
void loadLibraries(ManagerInterface* manager, const std::string& path,
                   const internal::DlSysInterface* sys = &internal::dlsys_impl);

} // namespace blobs
