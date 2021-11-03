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

#include <SecureBoot.hpp>
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

SecureBoot::SecureBoot(const std::string& name ,const std::string& securebootcurrentboot, const std::string& securebootmode,const  bool& securebootenable ,
                     sdbusplus::asio::object_server& objectServer,
                     std::shared_ptr<sdbusplus::asio::connection>& conn,
                     boost::asio::io_service& io) : 
	objServer(objectServer) ,name(name)
{
        (void)conn;
        (void)io;
	securebootInterface = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/SecureBoot/" + name,
			"xyz.openbmc_project.HostInventory.Item.SecureBoot");

	association = objectServer.add_interface(
			"/xyz/openbmc_project/HostInventory/SecureBoot/" + name,
			"org.openbmc.Associations");


	//setting proeproties of interface
	securebootInterface->register_property("SecureBootCurrentBoot", securebootcurrentboot);
	securebootInterface->register_property("SecureBootMode", securebootmode);
	securebootInterface->register_property("SecureBootEnable", securebootenable);

	if (!securebootInterface->initialize())
	{

		std::cerr << "error initializing secureboot Interface \n";
	}
}

SecureBoot::~SecureBoot()
{

	objServer.remove_interface(securebootInterface);
	objServer.remove_interface(association);
}


