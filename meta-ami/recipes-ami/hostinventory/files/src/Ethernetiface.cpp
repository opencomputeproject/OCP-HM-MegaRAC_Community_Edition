/*
// Copyright (c) 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include <unistd.h>

#include <Ethernetiface.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>


using namespace std;

Ethernetiface::Ethernetiface(const std::string& name , const std::string& linkstatus , const std::string& ipv4addr ,const std::string& MTUsize ,
		     const std::string& ipv6addr ,sdbusplus::asio::object_server& objectServer,
                     std::shared_ptr<sdbusplus::asio::connection>& conn,
                     boost::asio::io_service& io) : 
	objServer(objectServer) ,name(name)
{
	(void)conn;
	(void)io;
	ethInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/EthernetIfaces/" + name,
			"xyz.openbmc_project.HostInventory.Item.EthernetIface");

	association = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/EthernetIfaces/" + name,
			"org.openbmc.Associations");

	statusInterface = objectServer.add_interface(
                        "/xyz/openbmc_project/HostInventory/EthernetIfaces/" + name,
                        "xyz.openbmc_project.HostInventory.Item.Status");



	std::string state = "Enbaled";
	std::string health = "OK";

	//setting proeproties of interface
	ethInterface->register_property("Name", name);
	ethInterface->register_property("LinkStatus", linkstatus);
	ethInterface->register_property("MTUsize", MTUsize);
	ethInterface->register_property("IPV4Addr", ipv4addr);
	ethInterface->register_property("IPV6Addr", ipv6addr);

	if (!ethInterface->initialize())
	{

		std::cerr << "error initializing eth interface\n";
	}

        statusInterface->register_property("State", state);
        statusInterface->register_property("Health", health);

        if (!statusInterface->initialize())
        {

                std::cerr << "error initializing status interface\n";
        }



}

Ethernetiface::~Ethernetiface()
{

	objServer.remove_interface(ethInterface);
	objServer.remove_interface(association);
}


