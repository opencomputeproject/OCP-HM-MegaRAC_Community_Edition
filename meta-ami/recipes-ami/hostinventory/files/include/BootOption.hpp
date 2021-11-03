#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class BootOption 
{
  public:
    BootOption(const std::string& name,const std::string& bootoptionreference, const std::string& uefidevpath ,const std::string& displayname ,bool bootoptionenabled,
              sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~BootOption();



  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> bootoptionInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> statusInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};
