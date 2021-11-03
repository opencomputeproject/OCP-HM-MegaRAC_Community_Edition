#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class Storage 
{
  public:
    Storage(const std::string& name,const std::string& id, const std::string& description ,
              sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~Storage();



  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> storageInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};
