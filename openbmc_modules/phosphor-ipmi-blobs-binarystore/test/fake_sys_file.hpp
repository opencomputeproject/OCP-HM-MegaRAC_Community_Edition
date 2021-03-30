#pragma once

#include "sys_file.hpp"

#include <stdint.h>

#include <cstring>
#include <string>
#include <vector>

using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

using namespace std::string_literals;

namespace binstore
{

/* An in-memory file implementation to test read/write operations, which works
 * around the problem that Docker image cannot create file in tmpdir. */
class FakeSysFile : public SysFile
{
  public:
    FakeSysFile()
    {
    }
    FakeSysFile(const std::string& s) : data_(s)
    {
    }

    size_t readToBuf(size_t pos, size_t count, char* buf) const override
    {
        auto result = readAsStr(pos, count);
        std::copy(result.begin(), result.end(), buf);
        return result.size();
    }

    std::string readAsStr(size_t pos, size_t count) const override
    {
        if (pos >= data_.size())
        {
            return "";
        }

        return data_.substr(pos, count);
    }

    std::string readRemainingAsStr(size_t pos) const override
    {
        return readAsStr(pos, data_.size());
    }

    void writeStr(const std::string& data, size_t pos) override
    {
        if (pos >= data.size())
        {
            return;
        }

        data_.resize(pos);
        data_.insert(data_.begin() + pos, data.begin(), data.end());
    }

  protected:
    std::string data_ = ""s;
};

} // namespace binstore
