/*
// Copyright (c) 2018 Intel Corporation
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

#pragma once
#include "channel_layer.hpp"
#include "ipmid/api-types.hpp"

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/named_recursive_mutex.hpp>
#include <cstdint>
#include <ctime>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <variant>

namespace ipmi
{

using Json = nlohmann::json;

using DbusVariant = std::variant<std::vector<std::string>, std::string, bool>;

using DbusChObjProperties = std::vector<std::pair<std::string, DbusVariant>>;

static constexpr const char* ipmiChannelMutex = "ipmi_channel_mutex";
static constexpr const char* ipmiChMutexCleanupLockFile =
    "/var/lib/ipmi/ipmi_channel_mutex_cleanup";

/** @struct ChannelAccessData
 *
 *  Structure to store both non-volatile and volatile channel access information
 *  as used by IPMI specification (refer spec sec 22.22 to 22.24)
 */
struct ChannelAccessData
{
    ChannelAccess chNonVolatileData;
    ChannelAccess chVolatileData;
};

/** @struct ChannelProperties
 *
 *  Structure for channel information - base structure to get all information
 * about the channel.(refer spec sec 22.22 to 22.24)
 */
struct ChannelProperties
{
    std::string chName;
    uint8_t chID;
    bool isChValid;
    uint8_t activeSessCount;
    ChannelInfo chInfo;
    ChannelAccessData chAccess;
    size_t maxTransferSize;
};

class ChannelConfig;

ChannelConfig& getChannelConfigObject();

class ChannelConfig
{
  public:
    ChannelConfig(const ChannelConfig&) = delete;
    ChannelConfig& operator=(const ChannelConfig&) = delete;
    ChannelConfig(ChannelConfig&&) = delete;
    ChannelConfig& operator=(ChannelConfig&&) = delete;

    ~ChannelConfig();
    ChannelConfig();

    /** @brief determines valid channel
     *
     *  @param[in] chNum - channel number
     *
     *  @return true if valid, false otherwise
     */
    bool isValidChannel(const uint8_t chNum);

    /** @brief determines valid authentication type
     *
     *  @param[in] chNum - channel number
     *  @param[in] authType - authentication type
     *
     *  @return true if valid, false otherwise
     */
    bool isValidAuthType(const uint8_t chNum, const EAuthType& authType);

    /** @brief function to get channel name from channel number
     *
     *  @param[in] chNum - channel number index
     *
     *  @return network channel interface name
     */
    std::string getChannelName(const uint8_t chNum);

    /** @brief function to get channel number from channel name
     *
     *  @param[in] chName - channel name
     *
     *  @return network channel interface number
     */

    uint8_t getChannelByName(const std::string& chName)
    {
        return convertToChannelNumberFromChannelName(chName);
    }

    /** @brief determines supported session type of a channel
     *
     *  @param[in] chNum - channel number
     *
     *  @return EChannelSessSupported - supported session type
     */
    EChannelSessSupported getChannelSessionSupport(const uint8_t chNum);

    /** @brief determines number of active sessions on a channel
     *
     *  @param[in] chNum - channel number
     *
     *  @return numer of active sessions
     */
    int getChannelActiveSessions(const uint8_t chNum);

    /** @brief determines maximum transfer size for a channel
     *
     *  @param[in] chNum - channel number
     *
     *  @return maximum bytes that can be transferred on this channel
     */
    size_t getChannelMaxTransferSize(uint8_t chNum);

    /** @brief provides channel info details
     *
     *  @param[in] chNum - channel number
     *  @param[out] chInfo - channel info details
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc getChannelInfo(const uint8_t chNum, ChannelInfo& chInfo);

    /** @brief provides channel access data
     *
     *  @param[in] chNum - channel number
     *  @param[out] chAccessData - channel access data
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc getChannelAccessData(const uint8_t chNum, ChannelAccess& chAccessData);

    /** @brief to set channel access data
     *
     *  @param[in] chNum - channel number
     *  @param[in] chAccessData - channel access data
     *  @param[in] setFlag - flag to indicate updatable fields
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setChannelAccessData(const uint8_t chNum,
                            const ChannelAccess& chAccessData,
                            const uint8_t setFlag);

    /** @brief to get channel access data persistent data
     *
     *  @param[in] chNum - channel number
     *  @param[out] chAccessData - channel access data
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc getChannelAccessPersistData(const uint8_t chNum,
                                   ChannelAccess& chAccessData);

    /** @brief to set channel access data persistent data
     *
     *  @param[in] chNum - channel number
     *  @param[in] chAccessData - channel access data
     *  @param[in] setFlag - flag to indicate updatable fields
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc setChannelAccessPersistData(const uint8_t chNum,
                                   const ChannelAccess& chAccessData,
                                   const uint8_t setFlag);

    /** @brief provides supported authentication type for the channel
     *
     *  @param[in] chNum - channel number
     *  @param[out] authTypeSupported - supported authentication type
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc getChannelAuthTypeSupported(const uint8_t chNum,
                                   uint8_t& authTypeSupported);

    /** @brief provides enabled authentication type for the channel
     *
     *  @param[in] chNum - channel number
     *  @param[in] priv - privilege
     *  @param[out] authType - enabled authentication type
     *
     *  @return ccSuccess for success, others for failure.
     */
    Cc getChannelEnabledAuthType(const uint8_t chNum, const uint8_t priv,
                                 EAuthType& authType);

    /** @brief conver to channel privilege from system privilege
     *
     *  @param[in] value - privilege value
     *
     *  @return Channel privilege
     */
    CommandPrivilege convertToPrivLimitIndex(const std::string& value);

    /** @brief function to write persistent channel configuration to config file
     *
     *  @return 0 for success, -errno for failure.
     */
    int writeChannelPersistData();

    /** @brief function to write volatile channel configuration to config file
     *
     *  @return 0 for success, -errno for failure.
     */
    int writeChannelVolatileData();

  private:
    uint32_t signalFlag = 0;
    std::unique_ptr<boost::interprocess::named_recursive_mutex> channelMutex{
        nullptr};
    std::array<ChannelProperties, maxIpmiChannels> channelData;
    std::time_t nvFileLastUpdatedTime;
    std::time_t voltFileLastUpdatedTime;
    boost::interprocess::file_lock mutexCleanupLock;
    sdbusplus::bus::bus bus;
    bool signalHndlrObjectState = false;
    boost::interprocess::file_lock sigHndlrLock;

    /** @brief function to initialize persistent channel configuration
     *
     */
    void initChannelPersistData();

    /** @brief function to set default channel configuration based on channel
     * number
     *
     *  @param[in] chNum - channel number
     *  @param[in] chName - channel name
     */
    void setDefaultChannelConfig(const uint8_t chNum,
                                 const std::string& chName);

    /** @brief function to load all channel configuration
     *
     *  @return 0 for success, -errno for failure.
     */
    int loadChannelConfig();

    /** @brief function to read persistent channel data
     *
     *  @return 0 for success, -errno for failure.
     */
    int readChannelPersistData();

    /** @brief function to read volatile channel data
     *
     *  @return 0 for success, -errno for failure.
     */
    int readChannelVolatileData();

    /** @brief function to check and reload persistent channel data
     *
     *  @return 0 for success, -errno for failure.
     */
    int checkAndReloadNVData();

    /** @brief function to check and reload volatile channel data
     *
     *  @return 0 for success, -errno for failure.
     */
    int checkAndReloadVolatileData();

    /** @brief function to sync channel privilege with system network channel
     * privilege
     *
     *  @return 0 for success, -errno for failure.
     */
    int syncNetworkChannelConfig();

    /** @brief function to set D-Bus property value
     *
     *  @param[in] service - service name
     *  @param[in] objPath - object path
     *  @param[in] interface - interface
     *  @param[in] property - property name
     *  @param[in] value - property value
     *
     *  @return 0 for success, -errno for failure.
     */
    int setDbusProperty(const std::string& service, const std::string& objPath,
                        const std::string& interface,
                        const std::string& property, const DbusVariant& value);

    /** @brief function to get D-Bus property value
     *
     *  @param[in] service - service name
     *  @param[in] objPath - object path
     *  @param[in] interface - interface
     *  @param[in] property - property name
     *  @param[out] value - property value
     *
     *  @return 0 for success, -errno for failure.
     */
    int getDbusProperty(const std::string& service, const std::string& objPath,
                        const std::string& interface,
                        const std::string& property, DbusVariant& value);

    /** @brief function to read json config file
     *
     *  @param[in] configFile - configuration file name
     *
     *  @return Json object
     */
    Json readJsonFile(const std::string& configFile);

    /** @brief function to write json config file
     *
     *  @param[in] configFile - configuration file name
     *  @param[in] jsonData - json object
     *
     *  @return 0 for success, -errno for failure.
     */
    int writeJsonFile(const std::string& configFile, const Json& jsonData);

    /** @brief function to convert system access mode to Channel access mode
     * type
     *
     *  @param[in] mode - access mode in string
     *
     *  @return Channel access mode.
     */
    EChannelAccessMode convertToAccessModeIndex(const std::string& mode);

    /** @brief function to convert access mode value to string
     *
     *  @param[in] value - acess mode value
     *
     *  @return access mode in string
     */
    std::string convertToAccessModeString(const uint8_t value);

    /** @brief function to convert privilege value to string
     *
     *  @param[in] value - privilege value
     *
     *  @return privilege in string
     */
    std::string convertToPrivLimitString(const uint8_t value);

    /** @brief function to convert session support string to value type
     *
     *  @param[in] value - session support type in string
     *
     *  @return support session type
     */
    EChannelSessSupported
        convertToSessionSupportIndex(const std::string& value);

    /** @brief function to convert medium type string to value type
     *
     *  @param[in] value - medium type in string
     *
     *  @return channel medium type
     */
    EChannelMediumType convertToMediumTypeIndex(const std::string& value);

    /** @brief function to convert protocol type string to value type
     *
     *  @param[in] value - protocol type in string
     *
     *  @return channel protocol  type
     */
    EChannelProtocolType convertToProtocolTypeIndex(const std::string& value);

    /** @brief function to convert channel name to the IPMI channel number.
     *
     *  @param[in] chName - the channel name defined in the JSON input file
     *  (i.e. LAN1)
     *
     *  @return IPMI channel number
     */
    int convertToChannelNumberFromChannelName(const std::string& chName);

    /** @brief function to handle Channel access property update through the
     * D-Bus handler.
     *
     *  @param[in] path - D-Bus path to the network element (i.e. eth0)
     *  @param[in] chProperties - D-Bus channel properties
     */
    void processChAccessPropChange(const std::string& path,
                                   const DbusChObjProperties& chProperties);

    /** @brief function to retrieve last modification time for the named file
     *
     *  @param[in] fileName - the name of the file for which to acquire
     *  timestamp data
     *
     *  @return time the file was last modified
     */
    std::time_t getUpdatedFileTime(const std::string& fileName);

    /** @brief function to convert the DBus path to a network channel name
     *
     *  @param[in] path - The DBus path to the device
     *
     *  @return network channel name (i.e. eth0)
     */
    std::string getChannelNameFromPath(const std::string& path);
};

} // namespace ipmi
