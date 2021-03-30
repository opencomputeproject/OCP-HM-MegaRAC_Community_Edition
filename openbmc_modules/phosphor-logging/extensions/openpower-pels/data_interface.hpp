#pragma once

#include "dbus_types.hpp"
#include "dbus_watcher.hpp"

#include <filesystem>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

namespace openpower
{
namespace pels
{

/**
 * @class DataInterface
 *
 * A base class for gathering data about the system for use
 * in PELs. Implemented this way to facilitate mocking.
 */
class DataInterfaceBase
{
  public:
    DataInterfaceBase() = default;
    virtual ~DataInterfaceBase() = default;
    DataInterfaceBase(const DataInterfaceBase&) = default;
    DataInterfaceBase& operator=(const DataInterfaceBase&) = default;
    DataInterfaceBase(DataInterfaceBase&&) = default;
    DataInterfaceBase& operator=(DataInterfaceBase&&) = default;

    /**
     * @brief Returns the machine Type/Model
     *
     * @return string - The machine Type/Model string
     */
    virtual std::string getMachineTypeModel() const
    {
        return _machineTypeModel;
    }

    /**
     * @brief Returns the machine serial number
     *
     * @return string - The machine serial number
     */
    virtual std::string getMachineSerialNumber() const
    {
        return _machineSerialNumber;
    }

    /**
     * @brief Says if the system is managed by a hardware
     *        management console.
     * @return bool - If the system is HMC managed
     */
    virtual bool isHMCManaged() const
    {
        return _hmcManaged;
    }

    /**
     * @brief Says if the host is up and running
     *
     * @return bool - If the host is running
     */
    virtual bool isHostUp() const
    {
        return _hostUp;
    }

    using HostStateChangeFunc = std::function<void(bool)>;

    /**
     * @brief Register a callback function that will get
     *        called on all host on/off transitions.
     *
     * The void(bool) function will get passed the new
     * value of the host state.
     *
     * @param[in] name - The subscription name
     * @param[in] func - The function to run
     */
    void subscribeToHostStateChange(const std::string& name,
                                    HostStateChangeFunc func)
    {
        _hostChangeCallbacks[name] = func;
    }

    /**
     * @brief Unsubscribe from host state changes.
     *
     * @param[in] name - The subscription name
     */
    void unsubscribeFromHostStateChange(const std::string& name)
    {
        _hostChangeCallbacks.erase(name);
    }

    /**
     * @brief Returns the BMC firmware version
     *
     * @return std::string - The BMC version
     */
    virtual std::string getBMCFWVersion() const
    {
        return _bmcFWVersion;
    }

    /**
     * @brief Returns the server firmware version
     *
     * @return std::string - The server firmware version
     */
    virtual std::string getServerFWVersion() const
    {
        return _serverFWVersion;
    }

    /**
     * @brief Returns the BMC FW version ID
     *
     * @return std::string - The BMC FW version ID
     */
    virtual std::string getBMCFWVersionID() const
    {
        return _bmcFWVersionID;
    }

    /**
     * @brief Returns the process name given its PID.
     *
     * @param[in] pid - The PID value as a string
     *
     * @return std::optional<std::string> - The name, or std::nullopt
     */
    std::optional<std::string> getProcessName(const std::string& pid) const
    {
        namespace fs = std::filesystem;

        fs::path path{"/proc"};
        path /= fs::path{pid} / "exe";

        if (fs::exists(path))
        {
            return fs::read_symlink(path);
        }

        return std::nullopt;
    }

    /**
     * @brief Returns the 'send event logs to host' setting.
     *
     * @return bool - If sending PELs to the host is enabled.
     */
    virtual bool getHostPELEnablement() const
    {
        return _sendPELsToHost;
    }

    /**
     * @brief Returns the BMC state
     *
     * @return std::string - The BMC state property value
     */
    virtual std::string getBMCState() const
    {
        return _bmcState;
    }

    /**
     * @brief Returns the Chassis state
     *
     * @return std::string - The chassis state property value
     */
    virtual std::string getChassisState() const
    {
        return _chassisState;
    }

    /**
     * @brief Returns the chassis requested power
     *        transition value.
     *
     * @return std::string - The chassis transition property
     */
    virtual std::string getChassisTransition() const
    {
        return _chassisTransition;
    }

    /**
     * @brief Returns the Host state
     *
     * @return std::string - The Host state property value
     */
    virtual std::string getHostState() const
    {
        return _hostState;
    }

    /**
     * @brief Returns the motherboard CCIN
     *
     * @return std::string The motherboard CCIN
     */
    virtual std::string getMotherboardCCIN() const
    {
        return _motherboardCCIN;
    }

    /**
     * @brief Get the fields from the inventory necessary for doing
     *        a callout on an inventory path.
     *
     * @param[in] inventoryPath - The item to get the data for
     * @param[out] fruPartNumber - Filled in with the VINI/FN keyword
     * @param[out] ccin - Filled in with the VINI/CC keyword
     * @param[out] serialNumber - Filled in with the VINI/SN keyword
     */
    virtual void getHWCalloutFields(const std::string& inventoryPath,
                                    std::string& fruPartNumber,
                                    std::string& ccin,
                                    std::string& serialNumber) const = 0;

    /**
     * @brief Get the location code for an inventory item.
     *
     * @param[in] inventoryPath - The item to get the data for
     *
     * @return std::string - The location code
     */
    virtual std::string
        getLocationCode(const std::string& inventoryPath) const = 0;

    /**
     * @brief Get the list of system type names the system is called.
     *
     * @return std::vector<std::string> - The list of names
     */
    virtual const std::vector<std::string>& getSystemNames() const
    {
        return _systemNames;
    }

    /**
     * @brief Fills in the placeholder 'Ufcs' in the passed in location
     *        code with the machine feature code and serial number, which
     *        is needed to create a valid location code.
     *
     * @param[in] locationCode - Location code value starting with Ufcs-, and
     *                           if that isn't present it will be added first.
     *
     * @param[in] node - The node number the location is on.
     *
     * @return std::string - The expanded location code
     */
    virtual std::string expandLocationCode(const std::string& locationCode,
                                           uint16_t node) const = 0;

    /**
     * @brief Returns the inventory path for the FRU that the location
     *        code represents.
     *
     * @param[in] locationCode - Location code value starting with Ufcs-, and
     *                           if that isn't present it will be added first.
     *
     * @param[in] node - The node number the location is on.
     *
     * @return std::string - The inventory D-Bus object
     */
    virtual std::string
        getInventoryFromLocCode(const std::string& unexpandedLocationCode,
                                uint16_t node) const = 0;

  protected:
    /**
     * @brief Sets the host on/off state and runs any
     *        callback functions (if there was a change).
     */
    void setHostUp(bool hostUp)
    {
        if (_hostUp != hostUp)
        {
            _hostUp = hostUp;

            for (auto& [name, func] : _hostChangeCallbacks)
            {
                try
                {
                    func(_hostUp);
                }
                catch (std::exception& e)
                {
                    using namespace phosphor::logging;
                    log<level::ERR>("A host state change callback threw "
                                    "an exception");
                }
            }
        }
    }

    /**
     * @brief The machine type-model.  Always kept up to date
     */
    std::string _machineTypeModel;

    /**
     * @brief The machine serial number.  Always kept up to date
     */
    std::string _machineSerialNumber;

    /**
     * @brief The hardware management console status.  Always kept
     *        up to date.
     */
    bool _hmcManaged = false;

    /**
     * @brief The host up status.  Always kept up to date.
     */
    bool _hostUp = false;

    /**
     * @brief The map of host state change subscriber
     *        names to callback functions.
     */
    std::map<std::string, HostStateChangeFunc> _hostChangeCallbacks;

    /**
     * @brief The BMC firmware version string
     */
    std::string _bmcFWVersion;

    /**
     * @brief The server firmware version string
     */
    std::string _serverFWVersion;

    /**
     * @brief The BMC firmware version ID string
     */
    std::string _bmcFWVersionID;

    /**
     * @brief If sending PELs is enabled.
     *
     * This is usually set to false in manufacturing test.
     */
    bool _sendPELsToHost = true;

    /**
     * @brief The BMC state property
     */
    std::string _bmcState;

    /**
     * @brief The Chassis current power state property
     */
    std::string _chassisState;

    /**
     * @brief The Chassis requested power transition property
     */
    std::string _chassisTransition;

    /**
     * @brief The host state property
     */
    std::string _hostState;

    /**
     * @brief The motherboard CCIN
     */
    std::string _motherboardCCIN;

    /**
     * @brief The compatible system names array
     */
    std::vector<std::string> _systemNames;
};

/**
 * @class DataInterface
 *
 * Concrete implementation of DataInterfaceBase.
 */
class DataInterface : public DataInterfaceBase
{
  public:
    DataInterface() = delete;
    ~DataInterface() = default;
    DataInterface(const DataInterface&) = default;
    DataInterface& operator=(const DataInterface&) = default;
    DataInterface(DataInterface&&) = default;
    DataInterface& operator=(DataInterface&&) = default;

    /**
     * @brief Constructor
     *
     * @param[in] bus - The sdbusplus bus object
     */
    explicit DataInterface(sdbusplus::bus::bus& bus);

    /**
     * @brief Finds the D-Bus service name that hosts the
     *        passed in path and interface.
     *
     * @param[in] objectPath - The D-Bus object path
     * @param[in] interface - The D-Bus interface
     */
    DBusService getService(const std::string& objectPath,
                           const std::string& interface) const;

    /**
     * @brief Wrapper for the 'GetAll' properties method call
     *
     * @param[in] service - The D-Bus service to call it on
     * @param[in] objectPath - The D-Bus object path
     * @param[in] interface - The interface to get the props on
     *
     * @return DBusPropertyMap - The property results
     */
    DBusPropertyMap getAllProperties(const std::string& service,
                                     const std::string& objectPath,
                                     const std::string& interface) const;
    /**
     * @brief Wrapper for the 'Get' properties method call
     *
     * @param[in] service - The D-Bus service to call it on
     * @param[in] objectPath - The D-Bus object path
     * @param[in] interface - The interface to get the property on
     * @param[in] property - The property name
     * @param[out] value - Filled in with the property value.
     */
    void getProperty(const std::string& service, const std::string& objectPath,
                     const std::string& interface, const std::string& property,
                     DBusValue& value) const;

    /**
     * @brief Get the fields from the inventory necessary for doing
     *        a callout on an inventory path.
     *
     * @param[in] inventoryPath - The item to get the data for
     * @param[out] fruPartNumber - Filled in with the VINI/FN keyword
     * @param[out] ccin - Filled in with the VINI/CC keyword
     * @param[out] serialNumber - Filled in with the VINI/SN keyword
     */
    void getHWCalloutFields(const std::string& inventoryPath,
                            std::string& fruPartNumber, std::string& ccin,
                            std::string& serialNumber) const override;

    /**
     * @brief Get the location code for an inventory item.
     *
     * Throws an exception if the inventory item doesn't have the
     * location code interface.
     *
     * @param[in] inventoryPath - The item to get the data for
     *
     * @return std::string - The location code
     */
    std::string
        getLocationCode(const std::string& inventoryPath) const override;

    /**
     * @brief Fills in the placeholder 'Ufcs' in the passed in location
     *        code with the machine feature code and serial number, which
     *        is needed to create a valid location code.
     *
     * @param[in] locationCode - Location code value starting with Ufcs-, and
     *                           if that isn't present it will be added first.
     *
     * @param[in] node - The node number the location is one.
     *
     * @return std::string - The expanded location code
     */
    std::string expandLocationCode(const std::string& locationCode,
                                   uint16_t node) const override;

    /**
     * @brief Returns the inventory path for the FRU that the location
     *        code represents.
     *
     * @param[in] locationCode - Location code value starting with Ufcs-, and
     *                           if that isn't present it will be added first.
     *
     * @param[in] node - The node number the location is on.
     *
     * @return std::string - The inventory D-Bus object
     */
    std::string getInventoryFromLocCode(const std::string& expandedLocationCode,
                                        uint16_t node) const override;

  private:
    /**
     * @brief Reads the BMC firmware version string and puts it into
     *        _bmcFWVersion.
     */
    void readBMCFWVersion();

    /**
     * @brief Reads the server firmware version string and puts it into
     *        _serverFWVersion.
     */
    void readServerFWVersion();

    /**
     * @brief Reads the BMC firmware version ID and puts it into
     *        _bmcFWVersionID.
     */
    void readBMCFWVersionID();

    /**
     * @brief Reads the motherboard CCIN and puts it into _motherboardCCIN.
     *
     * It finds the motherboard first, possibly having to wait for it to
     * show up.
     */
    void readMotherboardCCIN();

    /**
     * @brief Finds all D-Bus paths that contain any of the interfaces
     *        passed in, by using GetSubTreePaths.
     *
     * @param[in] interfaces - The desired interfaces
     *
     * @return The D-Bus paths.
     */
    DBusPathList getPaths(const DBusInterfaceList& interfaces) const;

    /**
     * @brief The interfacesAdded callback used on the inventory to
     *        find the D-Bus object that has the motherboard interface.
     *        When the motherboard is found, it then adds a PropertyWatcher
     *        for the motherboard CCIN.
     */
    void motherboardIfaceAdded(sdbusplus::message::message& msg);

    /**
     * @brief Set the motherboard CCIN from the DBus variant that
     *        contains it.
     *
     * @param[in] ccin - The CCIN variant, a vector<uint8_t>.
     */
    void setMotherboardCCIN(const DBusValue& ccin)
    {
        const auto& c = std::get<std::vector<uint8_t>>(ccin);
        _motherboardCCIN = std::string{c.begin(), c.end()};
    }

    /**
     * @brief Adds the Ufcs- prefix to the location code passed in.
     *
     * Necessary because the location codes that come back from the
     * message registry and device callout JSON don't have it.
     *
     * @param[in] - The location code without a prefix, like P1-C1
     *
     * @return std::string - The location code with the prefix
     */
    static std::string addLocationCodePrefix(const std::string& locationCode);

    /**
     * @brief The D-Bus property or interface watchers that have callbacks
     *        registered that will set members in this class when
     *        they change.
     */
    std::vector<std::unique_ptr<DBusWatcher>> _properties;

    /**
     * @brief The sdbusplus bus object for making D-Bus calls.
     */
    sdbusplus::bus::bus& _bus;

    /**
     * @brief The interfacesAdded match object used to wait for inventory
     *        interfaces to show up, so that the object with the motherboard
     *        interface can be found.  After it is found, this object is
     *        deleted.
     */
    std::unique_ptr<sdbusplus::bus::match_t> _inventoryIfacesAddedMatch;
};

} // namespace pels
} // namespace openpower
