#include <boost/asio.hpp>
#include <memory>
#include <sdbusplus/asio/connection.hpp>

namespace
{

std::shared_ptr<boost::asio::io_context> ioCtx;
std::shared_ptr<sdbusplus::asio::connection> sdbusp;

} // namespace

void setIoContext(std::shared_ptr<boost::asio::io_context>& newIo)
{
    ioCtx = newIo;
}

std::shared_ptr<boost::asio::io_context> getIoContext()
{
    return ioCtx;
}

void setSdBus(std::shared_ptr<sdbusplus::asio::connection>& newBus)
{
    sdbusp = newBus;
}

std::shared_ptr<sdbusplus::asio::connection> getSdBus()
{
    return sdbusp;
}
