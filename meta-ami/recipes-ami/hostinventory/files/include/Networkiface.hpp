#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class Networkiface 
{
  public:
    Networkiface(const std::string& name ,const std::string& id , const std::string& state ,const std::string& health , 
              sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~Networkiface();


  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> networkInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> statusInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};
