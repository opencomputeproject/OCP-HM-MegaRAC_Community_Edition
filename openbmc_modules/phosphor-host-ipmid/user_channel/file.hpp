#pragma once

#include <stdio.h>

#include <experimental/filesystem>
namespace phosphor
{
namespace user
{

namespace fs = std::experimental::filesystem;

/** @class File
 *  @brief Responsible for handling file pointer
 *  Needed by putspent(3)
 */
class File
{
  private:
    /** @brief handler for operating on file */
    FILE* fp = NULL;

    /** @brief File name. Needed in the case where the temp
     *         needs to be removed
     */
    const std::string& name;

    /** @brief Should the file be removed at exit */
    bool removeOnExit = false;

  public:
    File() = delete;
    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&&) = delete;
    File& operator=(File&&) = delete;

    /** @brief Opens file and uses it to do file operation
     *
     *  @param[in] name         - File name
     *  @param[in] mode         - File open mode
     *  @param[in] removeOnExit - File to be removed at exit or no
     */
    File(const std::string& name, const std::string& mode,
         bool removeOnExit = false) :
        name(name),
        removeOnExit(removeOnExit)
    {
        fp = fopen(name.c_str(), mode.c_str());
    }

    /** @brief Opens file using provided file descriptor
     *
     *  @param[in] fd           - File descriptor
     *  @param[in] name         - File name
     *  @param[in] mode         - File open mode
     *  @param[in] removeOnExit - File to be removed at exit or no
     */
    File(int fd, const std::string& name, const std::string& mode,
         bool removeOnExit = false) :
        name(name),
        removeOnExit(removeOnExit)
    {
        fp = fdopen(fd, mode.c_str());
    }

    ~File()
    {
        if (fp)
        {
            fclose(fp);
        }

        // Needed for exception safety
        if (removeOnExit && fs::exists(name))
        {
            fs::remove(name);
        }
    }

    auto operator()()
    {
        return fp;
    }
};

} // namespace user
} // namespace phosphor
