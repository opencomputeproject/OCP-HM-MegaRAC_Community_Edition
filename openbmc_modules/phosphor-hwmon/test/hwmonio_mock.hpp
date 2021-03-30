#pragma once

#include "hwmonio.hpp"

#include <gmock/gmock.h>

namespace hwmonio
{

class HwmonIOMock : public HwmonIOInterface
{
  public:
    virtual ~HwmonIOMock(){};

    MOCK_CONST_METHOD5(read, int64_t(const std::string&, const std::string&,
                                     const std::string&, size_t,
                                     std::chrono::milliseconds));

    MOCK_CONST_METHOD6(write, void(uint32_t, const std::string&,
                                   const std::string&, const std::string&,
                                   size_t, std::chrono::milliseconds));

    MOCK_CONST_METHOD0(path, std::string());
};

} // namespace hwmonio

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
