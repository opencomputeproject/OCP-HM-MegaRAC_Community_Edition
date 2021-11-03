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

#include <Processor.hpp>
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

Processor::Processor(const std::string& name, const std::string& id,  const std::string& manufacturer,const uint64_t cores, const uint64_t threads ,const std::string& architecture ,
                     sdbusplus::asio::object_server& objectServer,
                     std::shared_ptr<sdbusplus::asio::connection>& conn,
                     boost::asio::io_service& io) : 
	objServer(objectServer) ,name(name)
{
        (void)conn;
        (void)io;
	ProcessorInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/Processors/" + name,
			"xyz.openbmc_project.HostInventory.Item.Processor");

	association = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/Processors/" + name,
			"org.openbmc.Associations");



	//setting interface proeprties
	ProcessorInterface->register_property("Name", name);
	ProcessorInterface->register_property("Id", id);
	ProcessorInterface->register_property("Manufacturer", manufacturer);
	ProcessorInterface->register_property("TotalCores", cores);
	ProcessorInterface->register_property("TotalThreads", threads);
	ProcessorInterface->register_property("ProcessorArchitecture", architecture);

	if (!ProcessorInterface->initialize())
	{

		std::cerr << "error initializing ProcessorInterface interface\n";
	}

}

Processor::~Processor()
{
	objServer.remove_interface(ProcessorInterface);
	objServer.remove_interface(association);
}


