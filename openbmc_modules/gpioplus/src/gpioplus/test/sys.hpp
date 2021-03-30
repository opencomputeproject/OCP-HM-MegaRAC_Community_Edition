#pragma once
#include <gmock/gmock.h>
#include <gpioplus/internal/sys.hpp>

namespace gpioplus
{
namespace test
{

class SysMock : public internal::Sys
{
  public:
    MOCK_CONST_METHOD2(open, int(const char*, int));
    MOCK_CONST_METHOD1(dup, int(int));
    MOCK_CONST_METHOD1(close, int(int));
    MOCK_CONST_METHOD3(read, int(int, void*, size_t));
    MOCK_CONST_METHOD2(fcntl_setfl, int(int, int));
    MOCK_CONST_METHOD1(fcntl_getfl, int(int));

    MOCK_CONST_METHOD2(gpiohandle_get_line_values,
                       int(int, struct gpiohandle_data*));
    MOCK_CONST_METHOD2(gpiohandle_set_line_values,
                       int(int, struct gpiohandle_data*));
    MOCK_CONST_METHOD2(gpio_get_chipinfo, int(int, struct gpiochip_info*));
    MOCK_CONST_METHOD2(gpio_get_lineinfo, int(int, struct gpioline_info*));
    MOCK_CONST_METHOD2(gpio_get_linehandle,
                       int(int, struct gpiohandle_request*));
    MOCK_CONST_METHOD2(gpio_get_lineevent, int(int, struct gpioevent_request*));
};

} // namespace test
} // namespace gpioplus
