#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class StorageDrive 
{
  public:
    StorageDrive(const std::string& name, const std::string& model, const std::string& serialno ,const std::string& state , const std::string& health ,
              const std::string& storageunit_name ,sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~StorageDrive();



  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> storagedriveInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> statusInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};
