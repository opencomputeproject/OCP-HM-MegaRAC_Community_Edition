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

#include <ProcessorSummary.hpp>
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

ProcessorSummary::ProcessorSummary(const std::string& name, const std::string& model ,const uint64_t count, const std::string& state , const std::string& health ,
                     sdbusplus::asio::object_server& objectServer,
                     std::shared_ptr<sdbusplus::asio::connection>& conn,
                     boost::asio::io_service& io) : 
	objServer(objectServer) ,name(name)
{
	(void)conn;
	(void)io;
	processorsummaryInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/ProcessorSummary/" + name ,
			"xyz.openbmc_project.HostInventory.Item.ProcessorSummary");

	association = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/ProcessorSummary/" + name ,
			"org.openbmc.Associations");


	//setting proeproties of interface
	processorsummaryInterface->register_property("Model", model);
	processorsummaryInterface->register_property("Count", count);
	processorsummaryInterface->register_property("Name", name );

	if (!processorsummaryInterface->initialize())
	{

		std::cerr << "error initializing processorsummary Interface \n";
	}

	statusInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/ProcessorSummary/" + name,
			"xyz.openbmc_project.HostInventory.Item.Status");

	statusInterface->register_property("State", state);
	statusInterface->register_property("Health", health);

	if (!statusInterface->initialize())
	{
		std::cerr << "error initializing status interface\n";
	}
}

ProcessorSummary::~ProcessorSummary()
{

	objServer.remove_interface(processorsummaryInterface);
	objServer.remove_interface(association);
}


