/********************************
 *PEF and Alerting Process
 *Author: Raghul R
 *Email : raghulr@ami.com
 *
 * ******************************/

#include "pef_action.hpp"

#include "pef_config_update.hpp"

#include <string>

static bool getPowerStatus()
{
    bool pwrGood = false;
    std::string pwrStatus;
    Value variant;
    try
    {
        auto method = conn->new_method_call(pwrService, pwrStateObjPath,
                                            PROP_INTF, METHOD_GET);
        method.append(pwrStateIface, "CurrentPowerState");
        auto reply = conn->call(method);
        reply.read(variant);
        pwrStatus = std::get<std::string>(variant);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get PEFControl Value",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        return pwrGood;
    }
    if (pwrStatus == "xyz.openbmc_project.State.Chassis.PowerState.On")
    {
        pwrGood = true;
    }
    return pwrGood;
}

static int initiateStateTransition(std::string powerAction)
{
    auto method =
        conn->new_method_call(pwrService, pwrCtlObjPath, PROP_INTF, METHOD_SET);
    method.append(pwrCtlIface, "RequestedHostTransition");
    method.append(std::variant<std::string>(powerAction.c_str()));

    auto reply = conn->call(method);

    if (reply.is_method_error())
    {
        std::cerr << "Failed to set RequestedHostTransition\n";
        return -1;
    }
    return 0;
}

static int initiateChassisStateTransition(std::string powerAction)
{
    auto method = conn->new_method_call(pwrService, pwrStateObjPath, PROP_INTF,
                                        METHOD_SET);
    method.append(pwrStateIface, "RequestedPowerTransition");
    method.append(std::variant<std::string>(powerAction.c_str()));

    auto reply = conn->call(method);

    if (reply.is_method_error())
    {
        std::cerr << "Failed to set RequestedPowerTransition\n";
        return -1;
    }
    return 0;
}

static bool checkSampleEvent(struct EventMsgData* eveMsgData)
{
    // sample event1
    if ((eveMsgData->sensorNum == 0x30) && (eveMsgData->sensorType == 0x01) &&
        ((eveMsgData->eventType & 0x7f) == 0x01) &&
        (eveMsgData->eventData[0] == 0x09) &&
        (eveMsgData->eventData[1] == 0xff) &&
        (eveMsgData->eventData[2] == 0xff))
    {
        return true;
    } // sample event2
    else if ((eveMsgData->sensorNum == 0x60) &&
             (eveMsgData->sensorType == 0x02) &&
             ((eveMsgData->eventType & 0x7f) == 0x01) &&
             (eveMsgData->eventData[0] == 0x02) &&
             (eveMsgData->eventData[1] == 0xff) &&
             (eveMsgData->eventData[2] == 0xff))
    {
        return true;
    } // sample event3
    else if ((eveMsgData->sensorNum == 0x53) &&
             (eveMsgData->sensorType == 0x0c) &&
             ((eveMsgData->eventType & 0x7f) == 0x6f) &&
             (eveMsgData->eventData[0] == 0x00) &&
             (eveMsgData->eventData[1] == 0xff) &&
             (eveMsgData->eventData[2] == 0xff))
    {
        return true;
    }

    return false;
}

static uint16_t sendSmtpAlert(std::string rec, struct EventMsgData* eveMsg,
                              uint8_t eveLog)
{
    std::string sensorPath = getPathFromSensorNumber(eveMsg->sensorNum);
    std::string sensorType = getSensorTypeStringFromPath(sensorPath.c_str());
    std::string sensorName;
    std::string severity;
    std::size_t found = sensorPath.find_last_of("/\\");
    sensorName = sensorPath.substr(found + 1);
    if (sensorName.empty())
    {
        sensorName = "unknown sensorName";
    }
    if (sensorType.empty())
    {
        sensorType = "unKnown sensorType";
    }

    uint8_t evnDat = (eveMsg->eventData[0] & 0x0F);
    bool assert = (eveMsg->eventType & 0x80) ? false : true;

    if (evnDat == 0x02 || evnDat == 0x09)
    {
        severity = "Critical";
    }
    else if (evnDat == 0x00 || evnDat == 0x07)
    {
        severity = "Warning";
    }

    if (!assert)
    {
        severity = "Ok";
    }

    std::string eventDataMsg = "unknown event";
    if (!(eveMsg->msgStr).empty())
    {
        eventDataMsg = eveMsg->msgStr;
    }
    else
    {
        uint8_t eveType = (eveMsg->eventType & 0x7f);

        if (eveType == static_cast<uint8_t>(EventTypeCode::threshold))
        {
            eventDataMsg = THRESHOLD_EVENT_TABLE.find(evnDat)->second;
        }
        else if (eveType == static_cast<uint8_t>(EventTypeCode::generic))
        {
            auto offset = GENERIC_EVENT_TABLE.find(eveMsg->sensorType)->second;
            eventDataMsg = offset.find(evnDat)->second;
        }
        else if (eveType ==
                 static_cast<uint8_t>(EventTypeCode::sensor_specific))
        {
            auto offset =
                SENSOR_SPECIFIC_EVENT_TABLE.find(eveMsg->sensorType)->second;
            eventDataMsg = offset.find(evnDat)->second;
        }
    }

    std::string hostName;
    std::string Subject;
    std::string Message;
    std::string alertSubject;
    try
    {
        Value variant;
        auto method =
            conn->new_method_call(pefBus, pefObj, PROP_INTF, METHOD_GET);
        method.append(pefConfInfoIntf, "Subject");
        auto reply = conn->call(method);
        if (reply.is_method_error())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to get Subject");
        }
        reply.read(variant);
        Subject = std::get<std::string>(variant);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get Subject");
    }

    try
    {
        Value variant;
        auto method = conn->new_method_call(networkService, networkObjPath,
                                            PROP_INTF, METHOD_GET);
        method.append(networkIface, "HostName");
        auto reply = conn->call(method);
        if (reply.is_method_error())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to get HostName method");
        }
        reply.read(variant);
        hostName = std::get<std::string>(variant);
        if (Subject.empty())
        {
            if (severity == "Ok")
            {
                alertSubject = "Message from " + hostName;
            }
            else
            {
                alertSubject = "Alert from " + hostName;
            }
        }
        else
        {
            alertSubject = Subject;
        }
    }
    catch (sdbusplus::exception_t& e)
    {
        alertSubject = "PEF Alert";
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get HostName");
    }

    std::string alertBody;

    try
    {
        Value variant;

        auto method =
            conn->new_method_call(pefBus, pefObj, PROP_INTF, METHOD_GET);
        method.append(pefConfInfoIntf, "Message");
        auto reply = conn->call(method);
        reply.read(variant);
        Message = std::get<std::string>(variant);
        alertBody = Message + "\r\n";
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get Message");
    }

    bool samEve = false;
    samEve = checkSampleEvent(eveMsg);
    if (samEve == true)
    {
        alertBody += "Sensor Name : Not Found";
    }
    else
    {
        alertBody += "Sensor Name : " + sensorName + "\r\n" +
                     "Sensor Type : " + sensorType + " \r\n" +
                     "Severity    : " + severity + "\r\n" +
                     "Description : " + eventDataMsg;
    }
    uint16_t mailstatus = 0;
    try
    {
        auto sendAlert = conn->new_method_call(mailService, mailObjPath,
                                               mailIface, sendMailMethod);
        sendAlert.append(rec, alertSubject.c_str(), alertBody.c_str());
        auto replyStatus = conn->call(sendAlert);
        if (replyStatus.is_method_error())
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to call send alert method");
        }
        replyStatus.read(mailstatus);
    }
    catch (sdbusplus::exception_t&)
    {
        std::cerr << "Failed to sendAlert\n";
        return -1;
    }

    return mailstatus;
}

std::vector<std::string> checkAlertPoicyTbl(int AlertPolicyNo)
{
    std::vector<std::string> matchedAltPolEntries;
    for (int index = 1; index <= ALERT_POLICY_SET; index++)
    {
        AlertPolicyTbl AlertPlyTbl;
        AlertPlyTbl = {};
        std::string AlertPlyObj =
            alertPolicyTableObj + std::to_string(AlertPolicyNo);
        try
        {
            PropertyMap alertPolicyValues;
            auto method = conn->new_method_call(pefBus, AlertPlyObj.c_str(),
                                                PROP_INTF, METHOD_GET_ALL);
            method.append(alertPolicyTableIntf);
            auto reply = conn->call(method);
            if (reply.is_method_error())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to get all Alert policy properties");
            }
            reply.read(alertPolicyValues);
            AlertPlyTbl.AlertNum =
                std::get<uint8_t>(alertPolicyValues.at("AlertNum"));
            // AlertPlyTbl.ChannelDestSel =
            // std::get<uint8_t>(alertPolicyValues.at("ChannelDestSel"));
            // AlertPlyTbl.AlertStingkey =
            // std::get<uint8_t>(alertPolicyValues.at("AlertStingkey"));
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to fetch Alert Policy Table Entries config",
                phosphor::logging::entry("EXCEPTION=%s", e.what()));
        }
        if (0 != (AlertPlyTbl.AlertNum & 0x08))
        {
            matchedAltPolEntries.push_back(AlertPlyObj.c_str());
        }

        AlertPolicyNo = AlertPolicyNo + NUM_OF_ALERT_POLICY;
    }

    return matchedAltPolEntries;
}

static void performPefAction(std::vector<std::string>& matEveFltEntries,
                             struct EventMsgData* eveMsg)
{
    for (int index = 0; index < matEveFltEntries.size(); index++)
    {
        EvtFilterTblEntry eveFltTblEntry;
        eveFltTblEntry = {};
        try
        {
            PropertyMap values;
            auto method =
                conn->new_method_call(pefBus, matEveFltEntries[index].c_str(),
                                      PROP_INTF, METHOD_GET_ALL);
            method.append(eventFilterTableIntf);
            auto reply = conn->call(method);
            if (reply.is_method_error())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to get all Event Filtering properties");
            }
            reply.read(values);
            eveFltTblEntry.EvtFilterAction =
                std::get<uint8_t>(values.at("EvtFilterAction"));
            eveFltTblEntry.AlertPolicyNum =
                std::get<uint8_t>(values.at("AlertPolicyNum"));
            eveFltTblEntry.EventSeverity =
                std::get<uint8_t>(values.at("EventSeverity"));
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to fetch Event Filtering Table Entries config",
                phosphor::logging::entry("EXCEPTION=%s", e.what()));
            return;
        }

        pefConfInfo pefcfgInfo;
        pefcfgInfo = {};

        try
        {
            PropertyMap pefCfgValues;
            auto method = conn->new_method_call(pefBus, pefObj, PROP_INTF,
                                                METHOD_GET_ALL);
            method.append(pefConfInfoIntf);
            auto reply = conn->call(method);
            if (reply.is_method_error())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to get all Event Filtering properties");
            }
            reply.read(pefCfgValues);
            pefcfgInfo.PEFControl =
                std::get<uint8_t>(pefCfgValues.at("PEFControl"));
            pefcfgInfo.PEFActionGblControl =
                std::get<uint8_t>(pefCfgValues.at("PEFActionGblControl"));
            pefcfgInfo.PEFStartupDly =
                std::get<uint8_t>(pefCfgValues.at("PEFStartupDly"));
            pefcfgInfo.PEFAlertStartupDly =
                std::get<uint8_t>(pefCfgValues.at("PEFAlertStartupDly"));
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to fetch pef conf info Entries config",
                phosphor::logging::entry("EXCEPTION=%s", e.what()));
            return;
        }

        if (((eveFltTblEntry.EvtFilterAction & POWER_OFF_ACTION) ==
             POWER_OFF_ACTION) ||
            ((eveFltTblEntry.EvtFilterAction & POWER_CYCLE_ACTION) ==
             POWER_CYCLE_ACTION) ||
            ((eveFltTblEntry.EvtFilterAction & RESET_ACTION) == RESET_ACTION))
        {
            if (((eveFltTblEntry.EvtFilterAction & POWER_OFF_ACTION) ==
                 POWER_OFF_ACTION) &&
                ((pefcfgInfo.PEFActionGblControl & POWER_OFF_ACTION) ==
                 POWER_OFF_ACTION))
            {
                int rc = initiateChassisStateTransition(pwrCtlOff);
                if (rc < 0)
                    std::cerr << "Failed to do power action\n";
            }
            else if ((((eveFltTblEntry.EvtFilterAction & POWER_CYCLE_ACTION) ==
                       POWER_CYCLE_ACTION) &&
                      ((pefcfgInfo.PEFActionGblControl & POWER_CYCLE_ACTION) ==
                       POWER_CYCLE_ACTION)) ||
                     (((eveFltTblEntry.EvtFilterAction & RESET_ACTION) ==
                       RESET_ACTION) &&
                      ((pefcfgInfo.PEFActionGblControl & RESET_ACTION) ==
                       RESET_ACTION)))
            {
                bool power = getPowerStatus();
                if (power == true)
                {
                    initiateStateTransition(pwrStateReset);
                }
                else
                {
                    std::cerr << "Failed to do power action\n";
                }
            }
        }

        if (((eveFltTblEntry.EvtFilterAction & ALERT_ACTION) == ALERT_ACTION) &&
            ((pefcfgInfo.PEFActionGblControl & ALERT_ACTION) == ALERT_ACTION))
        {
            int AlertpolNum = 0;
            AlertpolNum = eveFltTblEntry.AlertPolicyNum & 0x0F;
            std::vector<std::string> Alertpolicy{};
            Alertpolicy = checkAlertPoicyTbl(AlertpolNum);
            if (0 != Alertpolicy.size())
            {
                for (int AlertEntry = 0; AlertEntry < Alertpolicy.size();
                     AlertEntry++)
                {
                    AlertPolicyTbl AlertPlyTbl;
                    AlertPlyTbl = {};
                    try
                    {
                        PropertyMap alertPolicyValues;
                        auto method = conn->new_method_call(
                            pefBus, Alertpolicy[AlertEntry].c_str(), PROP_INTF,
                            METHOD_GET_ALL);
                        method.append(alertPolicyTableIntf);
                        auto reply = conn->call(method);
                        if (reply.is_method_error())
                        {
                            phosphor::logging::log<
                                phosphor::logging::level::ERR>(
                                "Failed to get all Alert policy properties");
                        }
                        reply.read(alertPolicyValues);
                        AlertPlyTbl.AlertNum =
                            std::get<uint8_t>(alertPolicyValues.at("AlertNum"));
                        // AlertPlyTbl.ChannelDestSel =
                        // std::get<uint8_t>(alertPolicyValues.at("ChannelDestSel"));
                        // AlertPlyTbl.AlertStingkey =
                        // std::get<uint8_t>(alertPolicyValues.at("AlertStingkey"));
                    }
                    catch (sdbusplus::exception_t& e)
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Failed to fetch Alert Policy Table Entries config",
                            phosphor::logging::entry("EXCEPTION=%s", e.what()));
                        return;
                    }

                    if (0 != (AlertPlyTbl.AlertNum & 0x08))
                    {
                        uint16_t alertStatus;
                        std::vector<std::string> recipient;
                        Value variant;

                        try
                        {
                            auto method = conn->new_method_call(
                                pefBus, pefObj, PROP_INTF, METHOD_GET);
                            method.append(pefConfInfoIntf, "Recipient");
                            auto reply = conn->call(method);
                            reply.read(variant);
                            recipient =
                                std::get<std::vector<std::string>>(variant);
                        }
                        catch (sdbusplus::exception_t& e)
                        {
                            phosphor::logging::log<
                                phosphor::logging::level::ERR>(
                                "Failed to get recipient",
                                phosphor::logging::entry("EXCEPTION=%s",
                                                         e.what()));
                            return;
                        }
                        for (int i = 0; i < recipient.size(); i++)
                        {
                            if (recipient[i].empty())
                            {
                                continue;
                            }
                            alertStatus = sendSmtpAlert(recipient[i], eveMsg,
                                                        pefcfgInfo.PEFControl);

                            if (alertStatus == 0)
                            {
                                phosphor::logging::log<
                                    phosphor::logging::level::INFO>(
                                    "Alert Send Sucessfully!!!");
                                try
                                {
                                    auto method = conn->new_method_call(
                                        pefBus, pefObj,
                                        "org.freedesktop.DBus.Properties",
                                        "Set");
                                    method.append(pefConfInfoIntf,
                                                  "LastBMCProcessedEventID");
                                    method.append(std::variant<uint16_t>(
                                        eveMsg->recordId));
                                    auto reply = conn->call(method);
                                }
                                catch (std::exception& e)
                                {
                                    phosphor::logging::log<
                                        phosphor::logging::level::ERR>(
                                        "Failed to set LastBMCProcessedEventID",
                                        phosphor::logging::entry("EXCEPTION=%s",
                                                                 e.what()));
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return;
}

static uint8_t pefEveDataMatch(uint8_t value, uint8_t andMask, uint8_t cmp1,
                               uint8_t cmp2)
{
    uint8_t match;
    uint8_t temp1, temp2, cmp2Flag = 1;

    temp1 = (value & andMask);

    if ((temp1 & cmp1) == (cmp2 & cmp1))
    {
        match = 1;
        if (ALL_BITS_MATCH_EXACT != cmp1)
        {
            temp2 = temp1 & ~cmp1;

            if (ALL_ZERO_BITS != cmp2)
            {
                if (0 == (temp2 & cmp2))
                {
                    match = 0;
                }
                else
                {
                    cmp2Flag = 0;
                }
            }

            if (cmp2Flag && (cmp2 != ALL_ONE_BITS))
            {
                if (0 == ((~(temp2 | cmp1) & ~cmp2) & 0xFF))
                {
                    match = 0;
                }
                else
                {
                    match = 1;
                }
            }
        }
    }
    else
    {
        match = 0;
    }
    return match;
}

static void eventFilteringProcess(struct EventMsgData* eventMsg)
{
    uint16_t OffsetMask = 1;
    std::vector<std::string> matchedEveFltEntries;
    for (int index = 1; index < MAX_EVT_FILTER_ENTRIES; index++)
    {
        OffsetMask = 1;
        EvtFilterTblEntry eveFltTblEntry;
        eveFltTblEntry = {};
        std::string eveFltEntryObj =
            eventFilterTableObj + std::to_string(index);
        try
        {
            // PropertyMap values =
            // getAllDbusProperties(bus,pefBus,eveFltEntryObj,eventFilterTableIntf);
            PropertyMap values;
            auto method = conn->new_method_call(pefBus, eveFltEntryObj.c_str(),
                                                PROP_INTF, METHOD_GET_ALL);
            method.append(eventFilterTableIntf);
            auto reply = conn->call(method);
            if (reply.is_method_error())
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Failed to get all properties");
            }
            reply.read(values);
            eveFltTblEntry.FilterConfig =
                std::get<uint8_t>(values.at("FilterConfig"));
            eveFltTblEntry.GenIDByte1 =
                std::get<uint8_t>(values.at("GenIDByte1"));
            eveFltTblEntry.GenIDByte2 =
                std::get<uint8_t>(values.at("GenIDByte2"));
            eveFltTblEntry.SensorType =
                std::get<uint8_t>(values.at("SensorType"));
            eveFltTblEntry.SensorNum =
                std::get<uint8_t>(values.at("SensorNum"));
            eveFltTblEntry.EventTrigger =
                std::get<uint8_t>(values.at("EventTrigger"));
            eveFltTblEntry.EventData1ANDMask =
                std::get<uint8_t>(values.at("EventData1ANDMask"));
            eveFltTblEntry.EventData1Cmp1 =
                std::get<uint8_t>(values.at("EventData1Cmp1"));
            eveFltTblEntry.EventData1Cmp2 =
                std::get<uint8_t>(values.at("EventData1Cmp2"));
            eveFltTblEntry.EventData2ANDMask =
                std::get<uint8_t>(values.at("EventData2ANDMask"));
            eveFltTblEntry.EventData2Cmp1 =
                std::get<uint8_t>(values.at("EventData2Cmp1"));
            eveFltTblEntry.EventData2Cmp2 =
                std::get<uint8_t>(values.at("EventData2Cmp2"));
            eveFltTblEntry.EventData3ANDMask =
                std::get<uint8_t>(values.at("EventData3ANDMask"));
            eveFltTblEntry.EventData3Cmp1 =
                std::get<uint8_t>(values.at("EventData3Cmp1"));
            eveFltTblEntry.EventData3Cmp2 =
                std::get<uint8_t>(values.at("EventData3Cmp2"));
            eveFltTblEntry.EventData1OffsetMask =
                std::get<uint16_t>(values.at("EventData1OffsetMask"));
        }
        catch (sdbusplus::exception_t& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to fetch Event Filtering Table Entry connfig",
                phosphor::logging::entry("EXCEPTION=%s", e.what()));
            continue;
        }

        // Check Event Filter is disabled
        if (0 == (eveFltTblEntry.FilterConfig & 0x80))
        {
            continue;
        }

        if (((eveFltTblEntry.GenIDByte1 != 0xFF) &&
             (eveFltTblEntry.GenIDByte1 != eventMsg->generatorId1)) ||
            ((eveFltTblEntry.GenIDByte2 != 0xFF) &&
             (eveFltTblEntry.GenIDByte2 != eventMsg->generatorId2)) ||
            (0 == eventMsg->sensorType) ||
            ((eveFltTblEntry.SensorType != 0xFF) &&
             (eveFltTblEntry.SensorType != eventMsg->sensorType)) ||
            ((eveFltTblEntry.SensorNum != 0xFF) &&
             (eveFltTblEntry.SensorNum != eventMsg->sensorNum)) ||
            ((eveFltTblEntry.EventTrigger != 0xFF) &&
             (eveFltTblEntry.EventTrigger != eventMsg->eventType)))
        {
            continue;
        }
        // check eventData1
        if (0 == pefEveDataMatch(eventMsg->eventData[0],
                                 eveFltTblEntry.EventData1ANDMask,
                                 eveFltTblEntry.EventData1Cmp1,
                                 eveFltTblEntry.EventData1Cmp2))
        {
            continue;
        }
        // check eventData2
        if (0 == pefEveDataMatch(eventMsg->eventData[1],
                                 eveFltTblEntry.EventData2ANDMask,
                                 eveFltTblEntry.EventData2Cmp1,
                                 eveFltTblEntry.EventData2Cmp2))
        {
            continue;
        }
        // check eventData3
        if (0 == pefEveDataMatch(eventMsg->eventData[2],
                                 eveFltTblEntry.EventData3ANDMask,
                                 eveFltTblEntry.EventData3Cmp1,
                                 eveFltTblEntry.EventData3Cmp2))
        {
            continue;
        }

        OffsetMask = OffsetMask << (eventMsg->eventData[0] & 0x0F);

        /* Check Data1OffsetMask */
        if (0 == (OffsetMask & eveFltTblEntry.EventData1OffsetMask))
        {
            continue;
        }
        matchedEveFltEntries.push_back(eveFltEntryObj.c_str());
    }

    if (0 != matchedEveFltEntries.size())
    {
        performPefAction(matchedEveFltEntries, eventMsg);
    }
    else
    {
        return;
    }
    return;
}

static void pefTask(const uint16_t& recId, const uint8_t& senType,
                    const uint8_t& senNum, const uint8_t& eveType,
                    const uint8_t& eveData1, const uint8_t& eveData2,
                    const uint8_t& eveData3, const uint16_t& genId,
                    const std::string& msgStr)
{
    EventMsgData eveMsg = {};
    eveMsg.recordId = recId;
    eveMsg.sensorType = senType;
    eveMsg.eventType = eveType;
    eveMsg.sensorNum = senNum;
    eveMsg.generatorId1 = ((genId >> 8) & 0xff);
    eveMsg.generatorId2 = (genId & 0xff);
    eveMsg.eventData[0] = eveData1;
    eveMsg.eventData[1] = eveData2;
    eveMsg.eventData[2] = eveData3;
    eveMsg.msgStr = msgStr;

    uint8_t pefCtl = 0;
    Value variant;
    try
    {
        auto method =
            conn->new_method_call(pefBus, pefObj, PROP_INTF, METHOD_GET);
        method.append(pefConfInfoIntf, "PEFControl");
        auto reply = conn->call(method);
        reply.read(variant);
        pefCtl = std::get<uint8_t>(variant);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get PEFControl Value",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        return;
    }

    uint8_t pefPostponeTimer = 0;
    Value var;
    try
    {
        auto method = conn->new_method_call(pefBus, pefPostponeTmrObj,
                                            PROP_INTF, METHOD_GET);
        method.append(pefPostponeTmrIface, "ArmPEFPostponeTmr");
        auto reply = conn->call(method);
        reply.read(var);
        pefPostponeTimer = std::get<uint8_t>(var);
    }
    catch (sdbusplus::exception_t& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to get PEFControl Value",
            phosphor::logging::entry("EXCEPTION=%s", e.what()));
        return;
    }

    if ((pefPostponeTimer == 0xFE) ||
        ((pefPostponeTimer != 0x00) && (pefPostponeTimer = !0xFF)))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "PEF Task is Disabled by Postpone Timer");
        return;
    }
    // If PEF Disabled
    if (0 == (pefCtl & 0x01))
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "PEF Action is Disabled");
        return;
    }

    eventFilteringProcess(&eveMsg);
    return;
}

int main()
{
    conn->request_name(pefEventFilteringBus);

    auto server = sdbusplus::asio::object_server(conn);

    std::shared_ptr<sdbusplus::asio::dbus_interface> pefTaskIface =
        server.add_interface(pefEventFilteringObj, pefTaskIntf);

    // Register doPefTask method
    pefTaskIface->register_method("doPefTask", pefTask);
    pefTaskIface->initialize();

    // Reguster getSensorNum and GetSensorName  method
    std::shared_ptr<sdbusplus::asio::dbus_interface> pefSetSensorIface =
        server.add_interface(pefSetSensorObj, pefSetSensorIntf);

    pefSetSensorIface->register_method("SetSensorNumber", SetSensorNumber);

    pefSetSensorIface->register_method("GetSensorName", GetSensorName);
    pefSetSensorIface->register_method("GetFilterEnable", GetFilterEnable);
    pefSetSensorIface->register_method("SetFilterEnable", SetFilterEnable);
    pefSetSensorIface->initialize();

    sdbusplus::bus::match::match EventFilterTableMonitor =
        startEventFilterTableMonitor(conn);
    sdbusplus::bus::match::match AlertPolicyTableMonitor =
        startAlertPolicyTableMonitor(conn);
    sdbusplus::bus::match::match PefConfInfoMonitor =
        startPefConfInfoMonitor(conn);
    sdbusplus::bus::match::match ArmPefPostponeTimerMonitor =
        startArmPefPostponeTimerMonitor(conn);

    io.run();
    return 0;
}
