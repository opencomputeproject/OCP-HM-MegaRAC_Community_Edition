#include <cstring>
#include <fcntl.h>
#include <gpioplus/chip.hpp>
#include <linux/gpio.h>
#include <string>
#include <system_error>

namespace gpioplus
{

LineFlags::LineFlags(uint32_t flags) :
    kernel(flags & GPIOLINE_FLAG_KERNEL), output(flags & GPIOLINE_FLAG_IS_OUT),
    active_low(flags & GPIOLINE_FLAG_ACTIVE_LOW),
    open_drain(flags & GPIOLINE_FLAG_OPEN_DRAIN),
    open_source(flags & GPIOLINE_FLAG_OPEN_SOURCE)
{
}

Chip::Chip(unsigned id, const internal::Sys* sys) :
    fd(std::string{"/dev/gpiochip"}.append(std::to_string(id)).c_str(),
       O_RDONLY | O_CLOEXEC, sys)
{
}

ChipInfo Chip::getChipInfo() const
{
    struct gpiochip_info info;
    memset(&info, 0, sizeof(info));

    int r = fd.getSys()->gpio_get_chipinfo(*fd, &info);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpio_get_chipinfo");
    }

    return ChipInfo{info.name, info.label, info.lines};
}

LineInfo Chip::getLineInfo(uint32_t offset) const
{
    struct gpioline_info info;
    memset(&info, 0, sizeof(info));
    info.line_offset = offset;

    int r = fd.getSys()->gpio_get_lineinfo(*fd, &info);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpio_get_lineinfo");
    }

    return LineInfo{info.flags, info.name, info.consumer};
}

const internal::Fd& Chip::getFd() const
{
    return fd;
}

} // namespace gpioplus
