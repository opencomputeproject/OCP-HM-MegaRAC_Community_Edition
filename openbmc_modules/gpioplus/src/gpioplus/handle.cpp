#include <cstring>
#include <gpioplus/handle.hpp>
#include <linux/gpio.h>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace gpioplus
{

HandleFlags::HandleFlags(LineFlags line_flags) :
    output(line_flags.output), active_low(line_flags.active_low),
    open_drain(line_flags.open_drain), open_source(line_flags.open_source)
{
}

uint32_t HandleFlags::toInt() const
{
    uint32_t ret = 0;
    if (output)
    {
        ret |= GPIOHANDLE_REQUEST_OUTPUT;
    }
    else
    {
        ret |= GPIOHANDLE_REQUEST_INPUT;
    }
    if (active_low)
    {
        ret |= GPIOHANDLE_REQUEST_ACTIVE_LOW;
    }
    if (open_drain)
    {
        ret |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
    }
    if (open_source)
    {
        ret |= GPIOHANDLE_REQUEST_OPEN_SOURCE;
    }
    return ret;
}

static int build(const Chip& chip, const std::vector<Handle::Line>& lines,
                 HandleFlags flags, std::string_view consumer_label)
{
    if (lines.size() > GPIOHANDLES_MAX)
    {
        throw std::runtime_error("Too many requested gpio handles");
    }

    struct gpiohandle_request req;
    memset(&req, 0, sizeof(req));
    for (size_t i = 0; i < lines.size(); ++i)
    {
        req.lineoffsets[i] = lines[i].offset;
        req.default_values[i] = lines[i].default_value;
    }
    req.flags = flags.toInt();
    if (consumer_label.size() >= sizeof(req.consumer_label))
    {
        throw std::invalid_argument("consumer_label");
    }
    memcpy(req.consumer_label, consumer_label.data(), consumer_label.size());
    req.lines = lines.size();

    int r = chip.getFd().getSys()->gpio_get_linehandle(*chip.getFd(), &req);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpio_get_linehandle");
    }

    return req.fd;
}

Handle::Handle(const Chip& chip, const std::vector<Line>& lines,
               HandleFlags flags, std::string_view consumer_label) :
    fd(build(chip, lines, flags, consumer_label), std::false_type(),
       chip.getFd().getSys()),
    nlines(lines.size())
{
}

const internal::Fd& Handle::getFd() const
{
    return fd;
}

std::vector<uint8_t> Handle::getValues() const
{
    std::vector<uint8_t> values(nlines);
    getValues(values);
    return values;
}

void Handle::getValues(std::vector<uint8_t>& values) const
{
    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    int r = fd.getSys()->gpiohandle_get_line_values(*fd, &data);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpiohandle_get_line_values");
    }

    values.resize(nlines);
    for (size_t i = 0; i < nlines; ++i)
    {
        values[i] = data.values[i];
    }
}

void Handle::setValues(const std::vector<uint8_t>& values) const
{
    if (values.size() != nlines)
    {
        throw std::runtime_error("Handle.setValues: Invalid input size");
    }

    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    for (size_t i = 0; i < nlines; ++i)
    {
        data.values[i] = values[i];
    }
    int r = fd.getSys()->gpiohandle_set_line_values(*fd, &data);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpiohandle_get_line_values");
    }
}

} // namespace gpioplus
