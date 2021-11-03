#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class Processor 
{
  public:
    Processor(const std::string& name, const std::string& id,  const std::string& manufacturer , const uint64_t cores, const uint64_t threads,const std::string& architecture ,
              sdbusplus::asio::object_server& objectServer,
              std::shared_ptr<sdbusplus::asio::connection>& conn,
              boost::asio::io_service& io);
    ~Processor();



  private:
    sdbusplus::asio::object_server& objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> ProcessorInterface;
    std::shared_ptr<sdbusplus::asio::dbus_interface> association;

    std::string name;
};

