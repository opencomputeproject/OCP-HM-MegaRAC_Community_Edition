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

#include <SubProcessor.hpp>
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



SubProcessor::SubProcessor(const std::string& name, const std::string& id ,const std::string& processortype, const uint64_t maxspeed ,const uint64_t threads,
			   const std::string& state , const std::string& health ,
                     	   sdbusplus::asio::object_server& objectServer,
                     	   std::shared_ptr<sdbusplus::asio::connection>& conn,
                     	   boost::asio::io_service& io) : 
	objServer(objectServer) ,name(name)
{
	(void)conn;
	(void)io;


	subprocessorInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/SubProcessors/" + name,
			"xyz.openbmc_project.HostInventory.Item.SubProcessor");

	association = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/SubProcessors/" +  name,
			"org.openbmc.Associations");

	
	statusInterface = nullptr;

	//setting interface proeprties
	subprocessorInterface->register_property("Name",name);
	subprocessorInterface->register_property("Id", id);
	subprocessorInterface->register_property("ProcessorType", processortype);
	subprocessorInterface->register_property("MaxSpeedMHz", maxspeed);
	subprocessorInterface->register_property("TotalThreads", threads);


	if (!subprocessorInterface->initialize())
	{

		std::cerr << "error initializing subprocessor Interface\n";
	}


        if( (processortype == "Thread")  && ((!state.empty()) && (!health.empty())))
        {

                statusInterface = objectServer.add_interface(
                                "/xyz/openbmc_project/HostInventory/SubProcessors/" + name,
                                "xyz.openbmc_project.HostInventory.Item.Status");
                statusInterface->register_property("State", state);
                statusInterface->register_property("Health", health);


                if (!statusInterface->initialize())
                {

                        std::cerr << "error initializing status interface\n";
                }

        }

}

SubProcessor::~SubProcessor()
{
	objServer.remove_interface(subprocessorInterface);
	objServer.remove_interface(association);
}
