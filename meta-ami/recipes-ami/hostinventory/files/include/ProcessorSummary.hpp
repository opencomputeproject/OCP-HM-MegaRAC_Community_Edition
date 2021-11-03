#pragma once

#include <optional>
#include <sdbusplus/asio/object_server.hpp>
#include <boost/asio/io_service.hpp>

class ProcessorSummary 
{
	public:
		ProcessorSummary(const std::string& name, const std::string& model ,const uint64_t count, const std::string& state , const std::string& health ,
				sdbusplus::asio::object_server& objectServer,
				std::shared_ptr<sdbusplus::asio::connection>& conn,
				boost::asio::io_service& io);
		~ProcessorSummary();



	private:
		sdbusplus::asio::object_server& objServer;
		std::shared_ptr<sdbusplus::asio::dbus_interface> processorsummaryInterface;
		std::shared_ptr<sdbusplus::asio::dbus_interface> statusInterface;
		std::shared_ptr<sdbusplus::asio::dbus_interface> association;

		std::string name;
};
