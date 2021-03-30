#pragma once

#include "image_handler.hpp"

#include <gmock/gmock.h>

namespace ipmi_flash
{

class ImageHandlerMock : public ImageHandlerInterface
{
  public:
    virtual ~ImageHandlerMock() = default;

    MOCK_METHOD1(open, bool(const std::string&));
    MOCK_METHOD0(close, void());
    MOCK_METHOD2(write, bool(std::uint32_t, const std::vector<std::uint8_t>&));
    MOCK_METHOD0(getSize, int());
};

} // namespace ipmi_flash
