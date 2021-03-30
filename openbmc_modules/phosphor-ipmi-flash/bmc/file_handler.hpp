#pragma once

#include "image_handler.hpp"

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace ipmi_flash
{

class FileHandler : public ImageHandlerInterface
{
  public:
    /**
     * Create a FileHandler.  This object is basically a filewriter.
     *
     * @param[in] filename - file to use for the contents, fully
     * qualified file system path.
     */
    explicit FileHandler(const std::string& filename) : filename(filename)
    {}

    bool open(const std::string& path) override;
    void close() override;
    bool write(std::uint32_t offset,
               const std::vector<std::uint8_t>& data) override;
    int getSize() override;

  private:
    /** the active hash path, ignore. */
    std::string path;

    /** The file handle. */
    std::ofstream file;

    /** The filename (including path) to use to write bytes. */
    std::string filename;
};

} // namespace ipmi_flash
