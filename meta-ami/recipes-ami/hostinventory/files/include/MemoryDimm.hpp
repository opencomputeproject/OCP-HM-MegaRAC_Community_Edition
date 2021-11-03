#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class MemoryDimm 
{
  public:
    MemoryDimm(const std::string& name,const std::string& manfac, const std::string& serial ,const std::string& partno ,
              sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~MemoryDimm();



  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> dimmInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> statusInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};
