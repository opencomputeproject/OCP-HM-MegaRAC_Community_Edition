/********************************
 *PEF Configuration Process
 *Author: Raghul R
 *Email : raghulr@ami.com
 *
 * ******************************/

#include <fstream>
#include <phosphor-logging/log.hpp>
#include <string>
#include <filesystem>
#include "pef_config.hpp"

Json parseJSONConfig(const std::string& configFile)
{
    std::ifstream jsonFile(configFile);
    if (!jsonFile.is_open())
    {
	    phosphor::logging::log<phosphor::logging::level::ERR>(
                "parseJSONConfig: Cannot open PEF config path");
    }
    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
	    phosphor::logging::log<phosphor::logging::level::ERR>(
                "parseJSONConfig: readings JSON parser failure");
    }

    return data;
}

void parsePefConfToDbus(std::shared_ptr<sdbusplus::asio::connection> conn,sdbusplus::asio::object_server& objectServer)
{
	try
	{
		auto data = parseJSONConfig(pefConfFilePath);
		for(const auto &pefConfData : data["PEFConfInfo"])
		{
			std::shared_ptr<sdbusplus::asio::dbus_interface> pefConfInfoIface = objectServer.add_interface(pefObj, pefConfInfoIntf);
			pefConfInfoIface->register_property("PEFControl", static_cast<uint8_t>(pefConfData["PEFControl"]),sdbusplus::asio::PropertyPermission::readWrite);
			pefConfInfoIface->register_property("PEFActionGblControl", static_cast<uint8_t>(pefConfData["PEFActionGblControl"]),sdbusplus::asio::PropertyPermission::readWrite);
    			pefConfInfoIface->register_property("PEFStartupDly", static_cast<uint8_t>(pefConfData["PEFStartupDly"]),sdbusplus::asio::PropertyPermission::readWrite);
    			pefConfInfoIface->register_property("PEFAlertStartupDly", static_cast<uint8_t>(pefConfData["PEFAlertStartupDly"]),sdbusplus::asio::PropertyPermission::readWrite);
			pefConfInfoIface->register_property("LastBMCProcessedEventID", static_cast<uint16_t>(pefConfData["LastBMCProcessedEventID"]),sdbusplus::asio::PropertyPermission::readWrite);
                        pefConfInfoIface->register_property("LastSWProcessedEventID", static_cast<uint16_t>(pefConfData["LastSWProcessedEventID"]),sdbusplus::asio::PropertyPermission::readWrite);
			pefConfInfoIface->initialize(true);
		}

		for(const auto &systemGuid : data["SystemGUID"])
		{
			std::shared_ptr<sdbusplus::asio::dbus_interface> systemGuidIface = objectServer.add_interface(pefObj, systemGUIDIntf);
			systemGuidIface->register_property("SystemGUID0" , static_cast<uint8_t>(systemGuid["SystemGUID0"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID1" , static_cast<uint8_t>(systemGuid["SystemGUID1"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID2" , static_cast<uint8_t>(systemGuid["SystemGUID2"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID3" , static_cast<uint8_t>(systemGuid["SystemGUID3"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID4" , static_cast<uint8_t>(systemGuid["SystemGUID4"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID5" , static_cast<uint8_t>(systemGuid["SystemGUID5"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID6" , static_cast<uint8_t>(systemGuid["SystemGUID6"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID7" , static_cast<uint8_t>(systemGuid["SystemGUID7"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID8" , static_cast<uint8_t>(systemGuid["SystemGUID8"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID9" , static_cast<uint8_t>(systemGuid["SystemGUID9"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID10" , static_cast<uint8_t>(systemGuid["SystemGUID10"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID11" , static_cast<uint8_t>(systemGuid["SystemGUID11"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID12" , static_cast<uint8_t>(systemGuid["SystemGUID12"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID13" , static_cast<uint8_t>(systemGuid["SystemGUID13"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID14" , static_cast<uint8_t>(systemGuid["SystemGUID14"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->register_property("SystemGUID15" , static_cast<uint8_t>(systemGuid["SystemGUID15"]),sdbusplus::asio::PropertyPermission::readWrite);
			systemGuidIface->initialize(true);
		}

		for(const auto &oemParamData : data["OEMParam"])
		{
			std::shared_ptr<sdbusplus::asio::dbus_interface> oemParamIface = objectServer.add_interface(pefObj, oemParamIntf);
			oemParamIface->register_property("OemParam0" , static_cast<uint8_t>(oemParamData["OEMParam0"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam1" , static_cast<uint8_t>(oemParamData["OEMParam1"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam2" , static_cast<uint8_t>(oemParamData["OEMParam2"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam3" , static_cast<uint8_t>(oemParamData["OEMParam3"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam4" , static_cast<uint8_t>(oemParamData["OEMParam4"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam5" , static_cast<uint8_t>(oemParamData["OEMParam5"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam6" , static_cast<uint8_t>(oemParamData["OEMParam6"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam7" , static_cast<uint8_t>(oemParamData["OEMParam7"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam8" , static_cast<uint8_t>(oemParamData["OEMParam8"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam9" , static_cast<uint8_t>(oemParamData["OEMParam9"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam10" , static_cast<uint8_t>(oemParamData["OEMParam10"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam11" , static_cast<uint8_t>(oemParamData["OEMParam11"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam12" , static_cast<uint8_t>(oemParamData["OEMParam12"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam13" , static_cast<uint8_t>(oemParamData["OEMParam13"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam14" , static_cast<uint8_t>(oemParamData["OEMParam14"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam15" , static_cast<uint8_t>(oemParamData["OEMParam15"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam16" , static_cast<uint8_t>(oemParamData["OEMParam16"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam17" , static_cast<uint8_t>(oemParamData["OEMParam17"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam18" , static_cast<uint8_t>(oemParamData["OEMParam18"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam19" , static_cast<uint8_t>(oemParamData["OEMParam19"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam20" , static_cast<uint8_t>(oemParamData["OEMParam20"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam21" , static_cast<uint8_t>(oemParamData["OEMParam21"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam22" , static_cast<uint8_t>(oemParamData["OEMParam22"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam23" , static_cast<uint8_t>(oemParamData["OEMParam23"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam24" , static_cast<uint8_t>(oemParamData["OEMParam24"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam25" , static_cast<uint8_t>(oemParamData["OEMParam25"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam26" , static_cast<uint8_t>(oemParamData["OEMParam26"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam27" , static_cast<uint8_t>(oemParamData["OEMParam27"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam28" , static_cast<uint8_t>(oemParamData["OEMParam28"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam29" , static_cast<uint8_t>(oemParamData["OEMParam29"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam30" , static_cast<uint8_t>(oemParamData["OEMParam30"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->register_property("OemParam31" , static_cast<uint8_t>(oemParamData["OEMParam31"]),sdbusplus::asio::PropertyPermission::readWrite);
			oemParamIface->initialize(true);
		}

		for(const auto &eventFilterTableData : data["EventFilterTable"])
		{
			int eventFltrEntry = 0;
			eventFltrEntry = eventFilterTableData["EventFilterTableEntry"];
			
			std::string eveObjName = eventFilterTableObj + std::to_string(eventFltrEntry);
			std::shared_ptr<sdbusplus::asio::dbus_interface> eventFilterTblIface = objectServer.add_interface(eveObjName.c_str() , eventFilterTableIntf);
			eventFilterTblIface->register_property("FilterConfig" , static_cast<uint8_t>(eventFilterTableData["FilterConfig"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EvtFilterAction" , static_cast<uint8_t>(eventFilterTableData["EvtFilterAction"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("AlertPolicyNum" , static_cast<uint8_t>(eventFilterTableData["AlertPolicyNum"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventSeverity" , static_cast<uint8_t>(eventFilterTableData["EventSeverity"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("GenIDByte1" , static_cast<uint8_t>(eventFilterTableData["GenIDByte1"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("GenIDByte2" , static_cast<uint8_t>(eventFilterTableData["GenIDByte2"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("SensorType" , static_cast<uint8_t>(eventFilterTableData["SensorType"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("SensorNum" , static_cast<uint8_t>(eventFilterTableData["SensorNum"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventTrigger" , static_cast<uint8_t>(eventFilterTableData["EventTrigger"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData1OffsetMask" , static_cast<uint16_t>(eventFilterTableData["EventData1OffsetMask"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData1ANDMask" , static_cast<uint8_t>(eventFilterTableData["EventData1ANDMask"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData1Cmp1" , static_cast<uint8_t>(eventFilterTableData["EventData1Cmp1"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData1Cmp2" , static_cast<uint8_t>(eventFilterTableData["EventData1Cmp2"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData2ANDMask" , static_cast<uint8_t>(eventFilterTableData["EventData2ANDMask"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData2Cmp1" , static_cast<uint8_t>(eventFilterTableData["EventData2Cmp1"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData2Cmp2" , static_cast<uint8_t>(eventFilterTableData["EventData2Cmp2"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData3ANDMask" , static_cast<uint8_t>(eventFilterTableData["EventData3ANDMask"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData3Cmp1" , static_cast<uint8_t>(eventFilterTableData["EventData3Cmp1"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->register_property("EventData3Cmp2" , static_cast<uint8_t>(eventFilterTableData["EventData3Cmp2"]),sdbusplus::asio::PropertyPermission::readWrite);
			eventFilterTblIface->initialize(true);
		}

		for(const auto &alertPolicyTblData : data["AlertPolicyTable"])
		{
			int alertPolicyEntry = 0; 
			alertPolicyEntry = alertPolicyTblData["AlertPolicyTableEntry"];
			std::string alertPolicyObjName = alertPolicyTableObj + std::to_string(alertPolicyEntry);
                        std::shared_ptr<sdbusplus::asio::dbus_interface>  alertPolicyTblIface = objectServer.add_interface(alertPolicyObjName , alertPolicyTableIntf);
                        alertPolicyTblIface->register_property("AlertNum",static_cast<uint8_t>(alertPolicyTblData["AlertNum"]),sdbusplus::asio::PropertyPermission::readWrite);
                        alertPolicyTblIface->register_property("ChannelDestSel",static_cast<uint8_t>(alertPolicyTblData["ChannelDestSel"]),sdbusplus::asio::PropertyPermission::readWrite);
                        alertPolicyTblIface->register_property("AlertStingkey",static_cast<uint8_t>(alertPolicyTblData["AlertStingkey"]),sdbusplus::asio::PropertyPermission::readWrite);
                        alertPolicyTblIface->initialize(true);
		}

		for(const auto &alertStringTblData : data["AlertStringTable"])
		{
			int alertStringEntry = 0; 
			alertStringEntry = alertStringTblData["AlertStringTableEntry"];
			std::string alertStrObjName = alertStringTableObj + std::to_string(alertStringEntry);
			std::shared_ptr<sdbusplus::asio::dbus_interface> alertStringTblIface = objectServer.add_interface(alertStrObjName , alertStringTableIntf);
			alertStringTblIface->register_property("EventFilterSel" , static_cast<uint8_t>(alertStringTblData["EventFilterSel"]),sdbusplus::asio::PropertyPermission::readWrite);
			alertStringTblIface->register_property("AlertStringSet" , static_cast<uint8_t>(alertStringTblData["AlertStringSet"]),sdbusplus::asio::PropertyPermission::readWrite);
			alertStringTblIface->register_property("AlertString0" , static_cast<uint16_t>(alertStringTblData["AlertString0"]),sdbusplus::asio::PropertyPermission::readWrite);
			alertStringTblIface->register_property("AlertString1" , static_cast<uint16_t>(alertStringTblData["AlertString1"]),sdbusplus::asio::PropertyPermission::readWrite);
			alertStringTblIface->register_property("AlertString2" , static_cast<uint16_t>(alertStringTblData["AlertString2"]),sdbusplus::asio::PropertyPermission::readWrite);
			alertStringTblIface->initialize(true);
		}
		
	}
	catch (nlohmann::json::exception &e)
    	{
        	phosphor::logging::log<phosphor::logging::level::ERR>(
            	"parsePefConf: Error parsing PEF config file");
            	return;
    	}
    	catch (std::out_of_range &e)
    	{
        	phosphor::logging::log<phosphor::logging::level::ERR>(
            	"parsePefConf: Error invalid type");
            	return;
    	}
    	return;

}

int main()
{
	boost::asio::io_service io;
	auto conn = std::make_shared<sdbusplus::asio::connection>(io);
	conn->request_name(pefBus);
	auto server = sdbusplus::asio::object_server(conn);

	std::shared_ptr<sdbusplus::asio::dbus_interface> pefPostponeTmrIface = server.add_interface(pefArmPostponeTmrObj, pefPostponeTmrIntf);         
	pefPostponeTmrIface->register_property("ArmPEFPostponeTmr", static_cast<uint8_t>(0),
					sdbusplus::asio::PropertyPermission::readWrite);
	pefPostponeTmrIface->initialize(true);

	std::shared_ptr<sdbusplus::asio::dbus_interface> pefCountdownTmrIface = server.add_interface(pefArmPostponeTmrObj, pefCountdownTmrIntf); 
        pefCountdownTmrIface->register_property("TmrCountdownValue", static_cast<uint8_t>(0),
					sdbusplus::asio::PropertyPermission::readWrite);
	pefCountdownTmrIface->initialize(true);

	parsePefConfToDbus(conn,server);
	io.run();
	return 0;
}
