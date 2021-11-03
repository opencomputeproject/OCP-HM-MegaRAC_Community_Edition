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
#include <MemoryDimm.hpp>
#include <Ethernetiface.hpp>
#include <BootOption.hpp>
#include <SecureBoot.hpp>
#include <Networkiface.hpp>
#include <Storage.hpp>
#include <StorageDrive.hpp>
#include <Processor.hpp>
#include <SubProcessor.hpp>
#include <ProcessorSummary.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <syslog.h>
#include <sys/inotify.h>
#include <unistd.h>

#define DEBUG

#ifdef DEBUG
#define DBGPRINT(X)  cout  << (X) << endl;
#else
#define DBGPRINT(X)  /* */
#endif



using namespace std;
using json = nlohmann::json;
constexpr char const *RedfishDirPath = "/etc/redfish/";

void createInvDev(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<MemoryDimm>>& dimms , json dimm_json)
{
	auto& dimm = dimms[dimm_json["Name"]];
	dimm = nullptr;
	//creating Dimm object
	dimm  = std::make_unique<MemoryDimm>(
			dimm_json["Name"],dimm_json["Manufacturer"],dimm_json["SerialNumber"],dimm_json["PartNumber"], objectServer, dbusConnection, io);

}



void createBootOption(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<BootOption>>& bootoptions, json boption_json)
{

	auto& bootoption = bootoptions[boption_json["Name"]];
	bootoption = nullptr;
	//creating BootOption  object
	bootoption  = std::make_unique<BootOption>(
			boption_json["Name"],boption_json["BootOptionReference"],boption_json["UefiDevicePath"],boption_json["DisplayName"],boption_json["BootOptionEnabled"], objectServer, dbusConnection, io);

}


void createEthDev(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<Ethernetiface>>& ethifaces , json eth_json)
{

	auto& eth = ethifaces[eth_json["Name"]];
	eth = nullptr;

	//removed becoz of errors eth_json["IPv6Addresses"][0]["Address"] eth_json["FQDN"]
	//creating ethiface object
	eth  = std::make_unique<Ethernetiface>(eth_json["Name"],eth_json["LinkStatus"],eth_json["IPv4Addresses"][0]["Address"], "FQDN","00:00:00:00:00:00", objectServer, dbusConnection, io);

}


void createNetDev(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<Networkiface>>& networkifaces , json net_json)
{

	auto& net = networkifaces[net_json["Name"]];
	net = nullptr;
	//creating ethiface object
	net  = std::make_unique<Networkiface>(net_json["Name"],net_json["Id"],net_json["Status"]["State"], net_json["Status"]["Health"], objectServer, dbusConnection, io);

}



void createStorageDev(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<Storage>>& storageunits , json storage_json)
{

	auto& storageunit = storageunits[storage_json["Name"]];
	storageunit = nullptr;
	//creating storage uint object
	storageunit  = std::make_unique<Storage>(storage_json["Name"],storage_json["Id"],storage_json["Description"] , objectServer, dbusConnection, io);

}

void createStorageDevDrive(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<StorageDrive>>& storageunitdrives , json storagedrive_json ,const std::string& storageunit_name )
{

	auto& storageunitdrive = storageunitdrives[storagedrive_json["Name"]];
	storageunitdrive = nullptr;
	//creating storage uint object
	storageunitdrive  = std::make_unique<StorageDrive>( storagedrive_json["Name"]  ,storagedrive_json["Model"],storagedrive_json["SerialNumber"] ,storagedrive_json["Status"]["State"],storagedrive_json["Status"]["Health"] , storageunit_name , objectServer, dbusConnection, io);

}


void createProcessor(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<Processor>>& processors , json processor_json)
{

	auto& processor = processors[processor_json["Name"]];
	processor = nullptr;
	//creating processor uint object
	processor  = std::make_unique<Processor>(processor_json["Name"],processor_json["Id"],processor_json["Manufacturer"] ,processor_json["TotalCores"],processor_json["TotalThreads"],processor_json["ProcessorArchitecture"] ,objectServer, dbusConnection, io);

}



void createSubProcessor(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<SubProcessor>>& subprocessors , json subprocessor_json  )
{
	auto& subprocessor = subprocessors[subprocessor_json["Name"]];
	subprocessor = nullptr;
	//creating subprocessor  object

	if((subprocessor_json["ProcessorType"]  == "Thread")  && ((!subprocessor_json["Status"]["State"].empty()) && (!subprocessor_json["Status"]["Health"].empty())))
	{
		subprocessor  = std::make_unique<SubProcessor>( subprocessor_json["Name"]  ,subprocessor_json["Id"],subprocessor_json["ProcessorType"] ,subprocessor_json["MaxSpeedMHz"],0,subprocessor_json["Status"]["State"],subprocessor_json["Status"]["Health"],  objectServer, dbusConnection, io);

	}

	else if(subprocessor_json["ProcessorType"]  == "Core")
	{

		subprocessor  = std::make_unique<SubProcessor>( subprocessor_json["Name"]  ,subprocessor_json["Id"],subprocessor_json["ProcessorType"] ,subprocessor_json["MaxSpeedMHz"], subprocessor_json["TotalThreads"], "" ,"", objectServer, dbusConnection, io);


	}

}
		


void createSecureBoot(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<SecureBoot>>& secureboots , json secureboot_json)
{

	auto& secureboot = secureboots["Secureboot"];
	secureboot = nullptr;
	//creating secureboot object
	secureboot = std::make_unique<SecureBoot>("Secureboot",secureboot_json["SecureBootCurrentBoot"],secureboot_json["SecureBootMode"],secureboot_json["SecureBootEnable"] , objectServer, dbusConnection, io);

}


void createProcessorSummary(boost::asio::io_service& io, sdbusplus::asio::object_server& objectServer,std::shared_ptr<sdbusplus::asio::connection>& dbusConnection ,
                  boost::container::flat_map<std::string, std::shared_ptr<ProcessorSummary>>& processorsummaries , json processorsummary_json)
{

	auto& processorsummary = processorsummaries["ProcessorSummary"];
	processorsummary = nullptr;

	//try {
	//creating processorsummary object
	//processorsummary = std::make_unique<ProcessorSummary>("ProcessorSummary",processorsummary_json["Model"],processorsummary_json["Count"],processorsummary_json["Status"]["State"],processorsummary_json["Status"]["Health"], objectServer, dbusConnection, io);

	processorsummary = std::make_unique<ProcessorSummary>("ProcessorSummary","Model",processorsummary_json["Count"],processorsummary_json["Status"]["State"],processorsummary_json["Status"]["Health"], objectServer, dbusConnection, io);
	
	//}

	/*catch (json::parse_error& e)
	{		
		DBGPRINT("###########Error : Hostinv service processorsummary obj creation parse error \n");
		DBGPRINT(e.what())
		DBGPRINT("###################################################################### \n");
	
	}

        catch (json::type_error& e)
        {
                DBGPRINT("###########Error : Hostinv service processorsummary obj creation type error \n");
                DBGPRINT(e.what())
                DBGPRINT("###################################################################### \n");

        }
        catch (json::out_of_range& e)
        {
                DBGPRINT("###########Error : Hostinv service processorsummary obj creation out_of_range  error \n");
                DBGPRINT(e.what())
                DBGPRINT("###################################################################### \n");

        }

        catch (json::other_error& e)
        {
                DBGPRINT("###########Error : Hostinv service processorsummary obj creation other error \n");
                DBGPRINT(e.what())
                DBGPRINT("###################################################################### \n");

        }

        catch (json::invalid_iterator& e)
        {
                DBGPRINT("###########Error : Hostinv service processorsummary obj creation invalid_iterator  error \n");
                DBGPRINT(e.what())
                DBGPRINT("###################################################################### \n");

        }*/


}



int main()
{
	int fd, wd, flag = -1;
	constexpr auto maxBytes = 1024;
	uint8_t buffer[maxBytes];

	//configuring usb gadget interface  
	//system("sh /usr/bin/configure_usb_gadget.sh");

	ofstream netwk_file;
        netwk_file.open("/etc/systemd/network/00-bmc-usb1.network");
	netwk_file << "[Match]" << endl; 	
	netwk_file << "Name=usb1" << endl;
	netwk_file << "[Network]" << endl;
	netwk_file << "Address=169.154.0.17/21" << endl;
	netwk_file.close();

	//system("systemctl restart xyz.openbmc_project.hostinventory.service");

	
	system("ifconfig usb1 169.154.0.17;ifconfig usb1 netmask 255.255.248.0"); 

	// Check if Redfish DIR exists.
	if (!std::filesystem::is_directory(RedfishDirPath))
	{
		std::filesystem::create_directories(RedfishDirPath);
	}


	//waiting for inventory json file
	fd = inotify_init();
	if (-1 == fd)
	{
		syslog(LOG_WARNING,"## inotify add watch failed \n");
		return -1;
	}

	wd = inotify_add_watch(fd, RedfishDirPath, IN_CLOSE_WRITE);
	if (-1 == wd)
	{
		syslog(LOG_WARNING,"## inotify add watch failed \n");
		return -1;
	}

	while (1)
	{
		auto bytes = read(fd, buffer, maxBytes);
		if (0 > bytes)
		{
			syslog(LOG_WARNING,"## read failed \n");
			return -1;
		}

		auto offset = 0;
		while (offset < bytes)
		{
			auto event = reinterpret_cast<inotify_event*>(&buffer[offset]);
			if ((event->mask & IN_CLOSE_WRITE) && !(event->mask & IN_ISDIR))
			{
				if (std::string(event->name) == "inventory.json")
				{
					flag = 1;
					break;
				}
			}

			offset += offsetof(inotify_event, name) + event->len;
		}

		if (flag == 1)
		{
			break;
		}
	}
	inotify_rm_watch(fd, wd);
	close(fd);

	//creating HostInventoryservice
	boost::asio::io_service io;
	auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
	DBGPRINT("###Debug : Hostinv service satrting \n");


	//Added new code  for removing extra lines (content-Disposition content-Type )in json file 
	//data will copied to new file and old file will be removed

	std::string line;
	std::ifstream fin;

	int line_no=0;

	fin.open("/etc/redfish/inventory.json");

	std::ofstream fout;
	fout.open("/etc/redfish/host-inventory.json");


	while (getline(fin, line))
	{
		line_no++;
		// write all lines to temp other than the lines with header deatails
		if( ((line.find("-----") != string::npos) && (line_no == 1 ))  || ((line.find("Content-") != string::npos) && (line_no == 2 )) || ((line.find("Content-") != string::npos) && (line_no == 3 )) )
			continue;
		else
			fout << line << std::endl;
	}

	fin.close();
	fout.close();


	remove("/etc/redfish/inventory.json");
	//new code ends here


	//checking for host inventory file 
	std::ifstream hinv_json_file("/etc/redfish/host-inventory.json");

	if( !hinv_json_file.good())
	{
		DBGPRINT("file open failed exiting from hostinv service \n");
		return -1;
	} 
	else
	{
		DBGPRINT("###Debug : Hostinv service, file open success\n");
	}

	DBGPRINT("###Debug : Hostinv service, inventory.json file good \n");

	//dbus objects for iventory devices
	boost::container::flat_map<std::string, std::shared_ptr<MemoryDimm>> dimms;
	boost::container::flat_map<std::string, std::shared_ptr<Ethernetiface>> ethifaces;
	boost::container::flat_map<std::string, std::shared_ptr<BootOption>> bootoptions;
	boost::container::flat_map<std::string, std::shared_ptr<SecureBoot>> secureboots;
	boost::container::flat_map<std::string, std::shared_ptr<Networkiface>> networkifaces;
	boost::container::flat_map<std::string, std::shared_ptr<Storage>> storageunits;
	boost::container::flat_map<std::string, std::shared_ptr<StorageDrive>> storageunitdrives;
	boost::container::flat_map<std::string, std::shared_ptr<Processor>> processors;
	boost::container::flat_map<std::string, std::shared_ptr<SubProcessor>> subprocessors;
	boost::container::flat_map<std::string, std::shared_ptr<ProcessorSummary>> processorsummaries;


	//name of the service	
	systemBus->request_name("xyz.openbmc_project.HostInventoryservice");
	sdbusplus::asio::object_server objectServer(systemBus);
	std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;

	DBGPRINT("###Debug : Hostinv service, Service name requested \n");

	//host inventory json data file, will get from host interface feature
	json inven_json;
	hinv_json_file >> inven_json;

	DBGPRINT("###Debug : Hostinv service, file copied to json object \n");

	//creating dimm objects from json file
	try 
	{	
		for(int i=0;i< inven_json["Systems"][0]["Memory"].size();i++)
		{

			createInvDev(io, objectServer, systemBus , dimms,inven_json["Systems"][0]["Memory"][i]);
		}	
	}
	catch(std::exception& e)
	{
		DBGPRINT("###########Error : Hostinv service InvDev obj creation error \n");
		DBGPRINT(e.what())

	}
	DBGPRINT("###Debug : Hostinv service, Created Mem devices objects \n");

	//creating ethernet objects from json file
	try
	{
		for(int k=0;k< inven_json["Systems"][0]["EthernetInterfaces"].size();k++)
		{

			createEthDev(io, objectServer, systemBus , ethifaces, inven_json["Systems"][0]["EthernetInterfaces"][k]);

		}
	}
	catch(std::exception& e)
	{
		DBGPRINT("###########Error : Hostinv service EthDev obj creation error \n");
		DBGPRINT(e.what())

	}


	DBGPRINT("###Debug : Hostinv service, Created Eth devices objects  \n");	


	//creating BootOptions object from json file
	try
	{
		for(int j=0;j< inven_json["Systems"][0]["Boot"]["BootOptions"].size();j++)
		{

			createBootOption(io, objectServer, systemBus , bootoptions,inven_json["Systems"][0]["Boot"]["BootOptions"][j]);
		}
	}
	catch(std::exception& e)
	{
		DBGPRINT("###########Error : Hostinv service BootOption obj creation error \n");
		DBGPRINT(e.what())

	}


	DBGPRINT("###Debug : Hostinv service, Created BootOptions devices objects  \n");

	//creating network objects from json file
	try
	{
		for(int l=0;l< inven_json["Systems"][0]["NetworkInterfaces"].size();l++)
		{

			createNetDev(io, objectServer, systemBus , networkifaces, inven_json["Systems"][0]["NetworkInterfaces"][l]);
		}
	}
	catch(std::exception& e)
	{
		DBGPRINT("###########Error : Hostinv service NetDev obj creation error \n");
		DBGPRINT(e.what())

	}

	DBGPRINT("###Debug : Hostinv service, Created Net devices objects  \n");

	//creating storage and drive objects from json file
	for(int m=0;m< inven_json["Systems"][0]["Storage"].size();m++)
	{


		try
		{
			createStorageDev(io, objectServer, systemBus , storageunits, inven_json["Systems"][0]["Storage"][m]);
		}
		catch(std::exception& e)
		{
			DBGPRINT("###########Error : Hostinv service StorageDev obj creation error \n");
			DBGPRINT(e.what())
				continue;

		}

		if(inven_json["Systems"][0]["Storage"][m]["Drives"].size() > 0 ) 
		{

			for(int n=0;n< inven_json["Systems"][0]["Storage"][m]["Drives"].size();n++)
			{
				try
				{

					createStorageDevDrive(io, objectServer, systemBus , storageunitdrives, inven_json["Systems"][0]["Storage"][m]["Drives"][n] ,inven_json["Systems"][0]["Storage"][m]["Name"]);
				}

				catch(std::exception& e)
				{
					DBGPRINT("###########Error : Hostinv service StorageDevDrive obj creation error \n");
					DBGPRINT(e.what())

				}

			}

		}

	}


	DBGPRINT("###Debug : Hostinv service, Created Storage devices and Drive objects  \n");
	//creating prcoessor and subprocessor objects
	for(int p=0;p< inven_json["Systems"][0]["Processors"].size();p++)
	{

		if(inven_json["Systems"][0]["Processors"][p]["Status"]["State"] != "Absent")
		{
			try
			{	
				createProcessor(io, objectServer, systemBus , processors, inven_json["Systems"][0]["Processors"][p]);
			}
			catch(std::exception& e)
			{
				DBGPRINT("###########Error : Hostinv service processor obj creation error \n");
				DBGPRINT(e.what())
					continue;

			}

			for(int q=0;q< inven_json["Systems"][0]["Processors"][p]["SubProcessors"].size();q++)
			{
				try
				{
					createSubProcessor(io, objectServer, systemBus , subprocessors, inven_json["Systems"][0]["Processors"][p]["SubProcessors"][q] );
				}
				catch(std::exception& e)
				{
					DBGPRINT("###########Error : Hostinv service Subprocessor obj creation error \n");
					DBGPRINT(e.what())
						continue;

				}


				for(int r=0;r< inven_json["Systems"][0]["Processors"][p]["SubProcessors"][q]["SubProcessors"].size();r++)
				{
					try
					{
						createSubProcessor(io, objectServer, systemBus , subprocessors, inven_json["Systems"][0]["Processors"][p]["SubProcessors"][q]["SubProcessors"][r] );
					}
					catch(std::exception& e)
					{
						DBGPRINT("###########Error : Hostinv service Subprocessor thread obj creation error \n");
						DBGPRINT(e.what())

					}


				}
			}
		}
		else
		{
			DBGPRINT("###Debug : Hostinv service, Prcoessor State Absent \n");

		}
	}
	DBGPRINT("###Debug : Hostinv service, created Processors and subprocessors objects  \n");

	//creating processorsummary object
	if( (inven_json["Systems"] != NULL ) && (inven_json["Systems"][0] != NULL) &&  ( inven_json["Systems"][0]["ProcessorSummary"] != NULL))
	{
		try
		{
			createProcessorSummary(io, objectServer, systemBus ,processorsummaries ,inven_json["Systems"][0]["ProcessorSummary"]);
		}
		catch (std::exception& e)
		{
			DBGPRINT("###########Error : Hostinv service processorsummary obj creation type error \n");
			DBGPRINT(e.what())

		}
	}


	DBGPRINT("###Debug : Hostinv service, created Processorsummary object  \n");
	//creating createSecureBoot
	if( (inven_json["Systems"] != NULL ) && (inven_json["Systems"][0] != NULL) && (inven_json["Systems"][0]["SecureBoot"] != NULL))
	{
		try
		{
			createSecureBoot(io, objectServer, systemBus ,secureboots ,inven_json["Systems"][0]["SecureBoot"]);
		}
		catch(std::exception& e)
		{
			DBGPRINT("###########Error : Hostinv service SecureBoot obj creation error \n");
			DBGPRINT(e.what())

		}

	}

	DBGPRINT("###Debug : Hostinv service, created Secureboot object  \n");
	//boost::asio::deadline_timer filterTimer(io);

	//running HostInventoryservice
	io.run();
}
