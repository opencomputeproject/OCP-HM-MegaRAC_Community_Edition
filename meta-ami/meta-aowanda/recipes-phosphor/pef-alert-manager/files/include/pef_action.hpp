#pragma once
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <sdrutils.hpp>
#include "pef_utils.hpp"

#define MAX_EVT_FILTER_ENTRIES 40
#define ALERT_POLICY_SET 4
#define NUM_OF_ALERT_POLICY 15

#define ALL_BITS_MATCH_EXACT 0xff
#define ALL_ZERO_BITS 0x00
#define ALL_ONE_BITS 0xff

#define ALERT_ACTION 0x01
#define NO_ALERT_ACTION 0x00

#define     DIAG_INT_ACTION         0x20
#define     OEM_ACTION              0x10
#define     POWER_CYCLE_ACTION      0x08
#define     RESET_ACTION            0x04
#define     POWER_OFF_ACTION        0x02
#define     ALERT_ACTION            0x01

using namespace std::chrono;

boost::asio::io_service io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

static constexpr const char* pefConfigFile = "/usr/share/pef-alert-manager/pef-alert-manager.json";
static constexpr const char* tmpConf = "/usr/share/pef-alert-manager/tmp.json";

/*pef configuration*/
static constexpr const char *pefEventFilteringBus = "xyz.openbmc_project.pef.alerting";
static constexpr const char *pefEventFilteringObj = "/xyz/openbmc_project/pef/alerting";
static constexpr const char *pefBus = "xyz.openbmc_project.pef.alert.manager";
static constexpr const char *pefObj = "/xyz/openbmc_project/PefAlertManager";
static constexpr const char *pefTaskIntf = "xyz.openbmc_project.pef.pefTask";
static constexpr const char *pefDbusIntf = "xyz.openbmc_project.pef.configurations";
static constexpr const char *pefConfInfoIntf = "xyz.openbmc_project.pef.PEFConfInfo";
static constexpr const char *pefSetSensorObj = "/xyz/openbmc_project/pef/alerting/SensorNumber";
static constexpr const char *pefSetSensorIntf = "xyz.openbmc_project.pef.alert.SensorNumber";
//static constexpr const char *systemGUIDIntf = "xyz.openbmc_project.pef.SystemGUID";
//static constexpr const char *oemParamIntf = "xyz.openbmc_project.pef.OEMParam";
static constexpr const char *eventFilterTableObj = "/xyz/openbmc_project/PefAlertManager/EventFilterTable/Entry";
static constexpr const char *eventFilterTableIntf = "xyz.openbmc_project.pef.EventFilterTable";
static constexpr const char *alertPolicyTableObj = "/xyz/openbmc_project/PefAlertManager/AlertPolicyTable/Entry";
static constexpr const char *alertPolicyTableIntf = "xyz.openbmc_project.pef.AlertPolicyTable";
//static constexpr const char *alertStringTableObj = "/xyz/openbmc_project/PefAlertManager/AlertStringTable/Entry";
//static constexpr const char *alertStringTableIntf = "xyz.openbmc_project.pef.AlertStringTable";
static constexpr const char *pefPostponeTmrObj = "/xyz/openbmc_project/PefAlertManager/ArmPostponeTimer";
static constexpr const char *pefPostponeTmrIface = "xyz.openbmc_project.pef.PEFPostponeTimer";
static constexpr const char *pefPostponeCountDownIface = "xyz.openbmc_project.pef.CountdownTmr";
/*mail alert*/
static constexpr const char* mailService = "xyz.openbmc_project.mail";
static constexpr const char* mailObjPath = "/xyz/openbmc_project/mail/alert";
static constexpr const char* mailIface = "xyz.openbmc_project.mail.alert";
static constexpr const char* sendMailMethod = "SendMail";
/*power status*/
static constexpr const char* pwrService = "xyz.openbmc_project.Chassis.Buttons";
static constexpr const char* pwrStateObjPath = "/xyz/openbmc_project/state/chassis0";
static constexpr const char* pwrStateIface = "xyz.openbmc_project.State.Chassis";
static constexpr const char* pwrStateReset = "xyz.openbmc_project.State.Host.Transition.Reboot";
/*power control*/
static constexpr const char* pwrCtlObjPath = "/xyz/openbmc_project/state/host0";
static constexpr const char* pwrCtlIface = "xyz.openbmc_project.State.Host";
static constexpr const char* pwrCtlOff = "xyz.openbmc_project.State.Chassis.Transition.Off";
/*Host Name*/
static constexpr const char* networkService = "xyz.openbmc_project.Network";
static constexpr const char* networkObjPath = "/xyz/openbmc_project/network/config";
static constexpr const char* networkIface = "xyz.openbmc_project.Network.SystemConfiguration";


struct EventMsgData
{
	uint16_t recordId;
	uint8_t generatorId1;
	uint8_t generatorId2;
	uint8_t sensorNum;
	uint8_t sensorType;
	uint8_t eventType;
	uint8_t eventData[2];
};

static void eventFilteringProcess(struct EventMsgData *eventMsg);

static uint8_t pefEveDataMatch(uint8_t ,uint8_t ,uint8_t, uint8_t);

static void performPefAction(std::vector<std::string>& ,struct EventMsgData *eveMsg);

static uint16_t sendSmtpAlert(struct EventMsgData *eveMsg,uint8_t);

static int initiateChassisStateTransition(std::string);

static int initiateStateTransition(std::string);

static bool getPowerStatus();

static bool checkSampleEvent(struct EventMsgData *eveMsgData);

enum class EventTypeCode : uint8_t
{
    threshold = 0x1,
    generic = 0x3,
    sensor_specific = 0x6f,
};

/* Map a event-byte to it's name */

const std::map<uint8_t,std::string>THRESHOLD_EVENT_TABLE = {
	{0x00,"lowerNonCritGoingLow"},
	{0x01,"lowerNonCritGoingHigh"},
	{0x02,"lowerCritGoingLow"},
	{0x03,"lowerCritGoingHigh"},
	{0x06,"upperNonCritGoingLow"},
	{0x07,"upperNonCritGoingHigh"},
	{0x08,"upperCritGoingLow"},
	{0x09,"upperCritGoingHigh"}};

const std::map<std::uint8_t, std::map<uint8_t, std::string>> SENSOR_SPECIFIC_EVENT_TABLE = {
    {0x0C, {{0x00, "CorrectableECC"},{0x01, "UncorrectableECC"},{0x02, "Parity"},{0x03,"MemoryScrubFailed"},{0x04,"MemoryDeviceDisabled"},{0x05,"CorrectableECClogging"},{0x06,"PresenceDetected"},{0x07,"ConfigurationError"},{0x08,"Spare"},{0x09,"Throttled"},{0x0a,"CriticalOvertemp"}}},
    {0x0D, {{0x00,"DrivePresent"},{0x01,"DriveFault"},{0x02,"PredictiveFailure"},{0x03,"HotSpare"},{0x04,"ParityCheck"},{0x05,"InCriticalArray"},{0x06,"InFailedArray"},{0x07,"RebuildInProgress"},{0x08,"RebuildAborted"}}},
    {0x05, {{0x00,"GenChassisIntrusion"},{0x01,"DriveBayIntrusion"},{0x02,"IOCardAreaIntrusion"},{0x03,"ProcessorAreaIntrusion"},{0x04,"LanLost"},{0x05,"UnauthorizedDock"},{0x06,"FanAreaIntrusion"}}},
    {0x07, {{0x00,"Ierr"},{0x01,"ThermalTrip"},{0x02,"Frb1"},{0x03,"Frb2"},{0x04,"Frb3"},{0x05,"ConfigurationError"},{0x06,"UncorrectableCpuComplexError"},{0x07,"ProcessorPresenceDetected"},{0x08,"ProcessorDisabled"},{0x09,"TerminatorPresenceDetected"},{0x0a,"ProcessorAutomaticallyThrottled"},{0x0b,"MachineCheckException"},{0x0c,"CorrectableMachineCheck"}}},
    {0x10, {{0x00,"Correctablememoryerror"},{0x01,"Eventloggingdisabled"},{0x02,"Logareareset"},{0x03,"Alleventloggingdisabled"},{0x04,"Logfull"},{0x05,"Logalmostfull"}}},
    {0x22, {{0x00,"S0_G0"},{0x01,"S1"},{0x02,"S2"},{0x03,"S3"},{0x04,"S4"},{0x05,"S5_G2"},{0x06,"S4_S5"},{0x07,"G3"},{0x08,"S1_S2_S3"},{0x09,"G1"},{0x0a,"S5"},{0x0b,"LegacyOn"},{0x0c,"LegacyOff"},{0x0e,"ACPI_Unknown"}}},
    {0x23, {{0x00,"Timerexpired"},{0x01,"Hardreset"},{0x02,"Powerdown"},{0x03,"Powercycle"},{0x08,"Timerinterrupt"}}}};

const std::map<std::uint8_t, std::map<uint8_t, std::string>> GENERIC_EVENT_TABLE = {
        {0x07, {{0x00,"ActiveStateLow"},{0x01,"ActiveStateHigh"}}},
        {0x01, {{0x00,"ActiveStateLow"},{0x01,"ActiveStateHigh"}}}};


static sdbusplus::bus::match::match startArmPefPostponeTimerMonitor(
    std::shared_ptr<sdbusplus::asio::connection> conn)
{
        auto PefPostponTmrMatcherCallback = [conn](
                                             sdbusplus::message::message &msg) {

		uint8_t timer = 0;
		std::string pefTmrIface;
		boost::container::flat_map<std::string, std::variant<uint8_t,uint16_t>>
        	                                propertiesChanged;
        	msg.read(pefTmrIface,propertiesChanged);
        	std::string property = propertiesChanged.begin()->first;
		timer = std::get<uint8_t>(propertiesChanged.begin()->second);
		if((timer == 0x00) || (timer == 0xFE) || (timer == 0xFF))
		{
			return;
		}

		while(timer != 0)
		{
			sleep(1);
			timer--;
			try
			{
				auto method = conn->new_method_call(pefBus,pefPostponeTmrObj, 
                                      "org.freedesktop.DBus.Properties", "Set");
				method.append(pefPostponeCountDownIface, "TmrCountdownValue");
    				method.append(std::variant<uint8_t>(timer));
	    			auto reply = conn->call(method);
			}
			catch (sdbusplus::exception_t& e)
        		{
                		phosphor::logging::log<phosphor::logging::level::ERR>("Failed to set TmrCountdownValue  Value",
                                                        phosphor::logging::entry("EXCEPTION=%s", e.what()));
        		}
		}
		try
		{
			auto method = conn->new_method_call(pefBus,pefPostponeTmrObj,
                        	              "org.freedesktop.DBus.Properties", "Set");
        		method.append(pefPostponeTmrIface, "ArmPEFPostponeTmr");
        		method.append(std::variant<uint8_t>(timer));
		 	auto reply = conn->call(method);
		}
		catch (sdbusplus::exception_t& e)
                {
  	               phosphor::logging::log<phosphor::logging::level::ERR>("Failed to set ArmPEFPostponeTmr  Value",
                                                        phosphor::logging::entry("EXCEPTION=%s", e.what()));
       		}
	};
	sdbusplus::bus::match::match PefPostponeTmrMatcher(
        static_cast<sdbusplus::bus::bus &>(*conn),
        "type='signal',interface='org.freedesktop.DBus.Properties',member='"
        "PropertiesChanged',arg0namespace='xyz.openbmc_project.pef."
        "PEFPostponeTimer'",
        std::move(PefPostponTmrMatcherCallback));
    return PefPostponeTmrMatcher;
}

static bool SetSensorNumber(int entry,std::string senType,std::string senName)
{
        std::string sensorObjPath;
        uint8_t senNum = 0;
        if((senType != "all_sensors") && (senName != "all_sensors"))
        {
                sensorObjPath = "/xyz/openbmc_project/sensors/" + senType + "/" + senName;
                senNum = getSensorNumberFromPath(sensorObjPath.c_str());
                if(senNum == 0xFF)
                {
                        return false;
                }
        }
        else
        {
                senNum = 0xFF;
        }
        std::string eveFltEntryObj = eventFilterTableObj + std::to_string(entry);
        auto method = conn->new_method_call(pefBus,eveFltEntryObj.c_str(),
                                      PROP_INTF, METHOD_SET);
        method.append(eventFilterTableIntf,"SensorNum");
        method.append(std::variant<uint8_t>(senNum));
        auto reply = conn->call(method);

        if (reply.is_method_error())
        {
		return false;
        }
        return true;
}

static std::vector<std::string> GetSensorName()
{
        std::string objPath,sensorName;
        std::vector<std::string> senNames;
        for(int entry = 1;entry <= MAX_EVT_FILTER_ENTRIES;entry++)
        {
                uint8_t sensorNum = 0;
                objPath.clear();
                sensorName.clear();
                std::string eveFltEntryObj = eventFilterTableObj + std::to_string(entry);
                try
                {
                        Value variant;
                        auto method = conn->new_method_call(pefBus,eveFltEntryObj.c_str(),
                                        PROP_INTF, METHOD_GET);
                        method.append(eventFilterTableIntf,"SensorNum");
                        auto reply = conn->call(method);
                        if (reply.is_method_error())
                        {
                                phosphor::logging::log<phosphor::logging::level::ERR>(
                                                "Failed to get SensorNum");
                                senNames.push_back(sensorName.c_str());
                                continue;
                        }
                        reply.read(variant);
                        sensorNum = std::get<std::uint8_t>(variant);
                }
                catch (sdbusplus::exception_t& e)
                {
			std::cerr << "Failed to get SensorNum";
                        senNames.push_back(sensorName.c_str());
                        continue;

                }
                if(sensorNum == 0xFF)
                {
                        senNames.push_back("all_sensors");
                        continue;
                }
                objPath = getPathFromSensorNumber(sensorNum);
                std::size_t found = objPath.find_last_of("/\\");
                sensorName = objPath.substr(found+1);
                senNames.push_back(sensorName.c_str());
        }
        return senNames;
}
