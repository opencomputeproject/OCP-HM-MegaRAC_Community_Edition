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

#include <Storage.hpp>
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

Storage::Storage(const std::string& name, const std::string& id, const std::string& description ,
                     sdbusplus::asio::object_server& objectServer,
                     std::shared_ptr<sdbusplus::asio::connection>& conn,
                     boost::asio::io_service& io) : 
	objServer(objectServer) ,name(name)
{
        (void)conn;
        (void)io;

	cout << "\n1 from Storage class \n";

	storageInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/Storages/" + name,
			"xyz.openbmc_project.HostInventory.Item.Storage");


	 cout << "2 from Storage class \n";

	association = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/Storages/" + name,
			"org.openbmc.Associations");

	cout << " 3from Storage class \n";

	//setting interface proeprties
	storageInterface->register_property("Name", name);
	storageInterface->register_property("Id", id);
	storageInterface->register_property("Description", description);

	cout << " 4from Storage class \n";


	if (!storageInterface->initialize())
	{

		std::cerr << "error initializing storage interface\n";
	}

}

Storage::~Storage()
{
	objServer.remove_interface(storageInterface);
	objServer.remove_interface(association);
}


