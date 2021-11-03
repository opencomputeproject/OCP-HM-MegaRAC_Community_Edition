#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class SecureBoot 
{
  public:
    SecureBoot(const std::string& name,const std::string& securebootcurrentboot, const std::string& securebootmode ,const bool& securebootenable ,
              sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~SecureBoot();



  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> securebootInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};
