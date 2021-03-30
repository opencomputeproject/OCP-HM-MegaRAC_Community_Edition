#include <cstring>
#include <gpioplus/event.hpp>
#include <linux/gpio.h>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace gpioplus
{

uint32_t EventFlags::toInt() const
{
    uint32_t ret = 0;
    if (rising_edge)
    {
        ret |= GPIOEVENT_REQUEST_RISING_EDGE;
    }
    if (falling_edge)
    {
        ret |= GPIOEVENT_REQUEST_FALLING_EDGE;
    }
    return ret;
}

static int build(const Chip& chip, uint32_t line_offset,
                 HandleFlags handle_flags, EventFlags event_flags,
                 std::string_view consumer_label)
{
    struct gpioevent_request req;
    memset(&req, 0, sizeof(req));
    req.lineoffset = line_offset;
    req.handleflags = handle_flags.toInt();
    req.eventflags = event_flags.toInt();
    if (consumer_label.size() >= sizeof(req.consumer_label))
    {
        throw std::invalid_argument("consumer_label");
    }
    memcpy(req.consumer_label, consumer_label.data(), consumer_label.size());

    int r = chip.getFd().getSys()->gpio_get_lineevent(*chip.getFd(), &req);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpio_get_lineevent");
    }

    return req.fd;
}

Event::Event(const Chip& chip, uint32_t line_offset, HandleFlags handle_flags,
             EventFlags event_flags, std::string_view consumer_label) :
    fd(build(chip, line_offset, handle_flags, event_flags, consumer_label),
       std::false_type(), chip.getFd().getSys())
{
}

const internal::Fd& Event::getFd() const
{
    return fd;
}

std::optional<Event::Data> Event::read() const
{
    struct gpioevent_data data;
    ssize_t read = fd.getSys()->read(*fd, &data, sizeof(data));
    if (read == -1)
    {
        if (errno == EAGAIN)
        {
            return std::nullopt;
        }
        throw std::system_error(errno, std::generic_category(),
                                "gpioevent read");
    }
    if (read != sizeof(data))
    {
        throw std::runtime_error("Event read didn't get enough data");
    }
    return Data{decltype(Data::timestamp)(data.timestamp), data.id};
}

uint8_t Event::getValue() const
{
    struct gpiohandle_data data;
    memset(&data, 0, sizeof(data));
    int r = fd.getSys()->gpiohandle_get_line_values(*fd, &data);
    if (r < 0)
    {
        throw std::system_error(-r, std::generic_category(),
                                "gpiohandle_get_line_values");
    }

    return data.values[0];
}

} // namespace gpioplus
