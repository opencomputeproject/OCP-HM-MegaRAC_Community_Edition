#include "config.h"

#include "associations.hpp"
#include "processing.hpp"
#include "src/argument.hpp"
#include "types.hpp"

#include <tinyxml2.h>

#include <atomic>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

AssociationMaps associationMaps;

static WhiteBlackList service_whitelist;
static WhiteBlackList service_blacklist;

/** Exception thrown when a path is not found in the object list. */
struct NotFoundException final : public sdbusplus::exception_t
{
    const char* name() const noexcept override
    {
        return "org.freedesktop.DBus.Error.FileNotFound";
    };
    const char* description() const noexcept override
    {
        return "path or object not found";
    };
    const char* what() const noexcept override
    {
        return "org.freedesktop.DBus.Error.FileNotFound: "
               "The requested object was not found";
    };
};

void update_owners(sdbusplus::asio::connection* conn,
                   boost::container::flat_map<std::string, std::string>& owners,
                   const std::string& new_object)
{
    if (boost::starts_with(new_object, ":"))
    {
        return;
    }
    conn->async_method_call(
        [&, new_object](const boost::system::error_code ec,
                        const std::string& nameOwner) {
            if (ec)
            {
                std::cerr << "Error getting owner of " << new_object << " : "
                          << ec << "\n";
                return;
            }
            owners[nameOwner] = new_object;
        },
        "org.freedesktop.DBus", "/", "org.freedesktop.DBus", "GetNameOwner",
        new_object);
}

void send_introspection_complete_signal(sdbusplus::asio::connection* system_bus,
                                        const std::string& process_name)
{
    // TODO(ed) This signal doesn't get exposed properly in the
    // introspect right now.  Find out how to register signals in
    // sdbusplus
    sdbusplus::message::message m = system_bus->new_signal(
        MAPPER_PATH, "xyz.openbmc_project.ObjectMapper.Private",
        "IntrospectionComplete");
    m.append(process_name);
    m.signal_send();
}

struct InProgressIntrospect
{
    InProgressIntrospect(
        sdbusplus::asio::connection* system_bus, boost::asio::io_service& io,
        const std::string& process_name, AssociationMaps& am
#ifdef DEBUG
        ,
        std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>>
            global_start_time
#endif
        ) :
        system_bus(system_bus),
        io(io), process_name(process_name), assocMaps(am)
#ifdef DEBUG
        ,
        global_start_time(global_start_time),
        process_start_time(std::chrono::steady_clock::now())
#endif
    {
    }
    ~InProgressIntrospect()
    {
        send_introspection_complete_signal(system_bus, process_name);

#ifdef DEBUG
        std::chrono::duration<float> diff =
            std::chrono::steady_clock::now() - process_start_time;
        std::cout << std::setw(50) << process_name << " scan took "
                  << diff.count() << " seconds\n";

        // If we're the last outstanding caller globally, calculate the
        // time it took
        if (global_start_time != nullptr && global_start_time.use_count() == 1)
        {
            diff = std::chrono::steady_clock::now() - *global_start_time;
            std::cout << "Total scan took " << diff.count()
                      << " seconds to complete\n";
        }
#endif
    }
    sdbusplus::asio::connection* system_bus;
    boost::asio::io_service& io;
    std::string process_name;
    AssociationMaps& assocMaps;
#ifdef DEBUG
    std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>>
        global_start_time;
    std::chrono::time_point<std::chrono::steady_clock> process_start_time;
#endif
};

void do_associations(sdbusplus::asio::connection* system_bus,
                     interface_map_type& interfaceMap,
                     sdbusplus::asio::object_server& objectServer,
                     const std::string& processName, const std::string& path)
{
    system_bus->async_method_call(
        [&objectServer, path, processName, &interfaceMap](
            const boost::system::error_code ec,
            const std::variant<std::vector<Association>>& variantAssociations) {
            if (ec)
            {
                std::cerr << "Error getting associations from " << path << "\n";
            }
            std::vector<Association> associations =
                std::get<std::vector<Association>>(variantAssociations);
            associationChanged(objectServer, associations, path, processName,
                               interfaceMap, associationMaps);
        },
        processName, path, "org.freedesktop.DBus.Properties", "Get",
        assocDefsInterface, assocDefsProperty);
}

void do_introspect(sdbusplus::asio::connection* system_bus,
                   std::shared_ptr<InProgressIntrospect> transaction,
                   interface_map_type& interface_map,
                   sdbusplus::asio::object_server& objectServer,
                   std::string path, int timeoutRetries = 0)
{
    constexpr int maxTimeoutRetries = 3;
    system_bus->async_method_call(
        [&interface_map, &objectServer, transaction, path, system_bus,
         timeoutRetries](const boost::system::error_code ec,
                         const std::string& introspect_xml) {
            if (ec)
            {
                if (ec.value() == boost::system::errc::timed_out &&
                    timeoutRetries < maxTimeoutRetries)
                {
                    do_introspect(system_bus, transaction, interface_map,
                                  objectServer, path, timeoutRetries + 1);
                    return;
                }
                std::cerr << "Introspect call failed with error: " << ec << ", "
                          << ec.message()
                          << " on process: " << transaction->process_name
                          << " path: " << path << "\n";
                return;
            }

            tinyxml2::XMLDocument doc;

            tinyxml2::XMLError e = doc.Parse(introspect_xml.c_str());
            if (e != tinyxml2::XMLError::XML_SUCCESS)
            {
                std::cerr << "XML parsing failed\n";
                return;
            }

            tinyxml2::XMLNode* pRoot = doc.FirstChildElement("node");
            if (pRoot == nullptr)
            {
                std::cerr << "XML document did not contain any data\n";
                return;
            }
            auto& thisPathMap = interface_map[path];
            tinyxml2::XMLElement* pElement =
                pRoot->FirstChildElement("interface");
            while (pElement != nullptr)
            {
                const char* iface_name = pElement->Attribute("name");
                if (iface_name == nullptr)
                {
                    continue;
                }

                thisPathMap[transaction->process_name].emplace(iface_name);

                if (std::strcmp(iface_name, assocDefsInterface) == 0)
                {
                    do_associations(system_bus, interface_map, objectServer,
                                    transaction->process_name, path);
                }

                pElement = pElement->NextSiblingElement("interface");
            }

            // Check if this new path has a pending association that can
            // now be completed.
            checkIfPendingAssociation(path, interface_map,
                                      transaction->assocMaps, objectServer);

            pElement = pRoot->FirstChildElement("node");
            while (pElement != nullptr)
            {
                const char* child_path = pElement->Attribute("name");
                if (child_path != nullptr)
                {
                    std::string parent_path(path);
                    if (parent_path == "/")
                    {
                        parent_path.clear();
                    }

                    do_introspect(system_bus, transaction, interface_map,
                                  objectServer, parent_path + "/" + child_path);
                }
                pElement = pElement->NextSiblingElement("node");
            }
        },
        transaction->process_name, path, "org.freedesktop.DBus.Introspectable",
        "Introspect");
}

void start_new_introspect(
    sdbusplus::asio::connection* system_bus, boost::asio::io_service& io,
    interface_map_type& interface_map, const std::string& process_name,
    AssociationMaps& assocMaps,
#ifdef DEBUG
    std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>>
        global_start_time,
#endif
    sdbusplus::asio::object_server& objectServer)
{
    if (needToIntrospect(process_name, service_whitelist, service_blacklist))
    {
        std::shared_ptr<InProgressIntrospect> transaction =
            std::make_shared<InProgressIntrospect>(system_bus, io, process_name,
                                                   assocMaps
#ifdef DEBUG
                                                   ,
                                                   global_start_time
#endif
            );

        do_introspect(system_bus, transaction, interface_map, objectServer,
                      "/");
    }
}

// TODO(ed) replace with std::set_intersection once c++17 is available
template <class InputIt1, class InputIt2>
bool intersect(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
    while (first1 != last1 && first2 != last2)
    {
        if (*first1 < *first2)
        {
            ++first1;
            continue;
        }
        if (*first2 < *first1)
        {
            ++first2;
            continue;
        }
        return true;
    }
    return false;
}

void doListNames(
    boost::asio::io_service& io, interface_map_type& interface_map,
    sdbusplus::asio::connection* system_bus,
    boost::container::flat_map<std::string, std::string>& name_owners,
    AssociationMaps& assocMaps, sdbusplus::asio::object_server& objectServer)
{
    system_bus->async_method_call(
        [&io, &interface_map, &name_owners, &objectServer, system_bus,
         &assocMaps](const boost::system::error_code ec,
                     std::vector<std::string> process_names) {
            if (ec)
            {
                std::cerr << "Error getting names: " << ec << "\n";
                std::exit(EXIT_FAILURE);
                return;
            }
            // Try to make startup consistent
            std::sort(process_names.begin(), process_names.end());
#ifdef DEBUG
            std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>>
                global_start_time = std::make_shared<
                    std::chrono::time_point<std::chrono::steady_clock>>(
                    std::chrono::steady_clock::now());
#endif
            for (const std::string& process_name : process_names)
            {
                if (needToIntrospect(process_name, service_whitelist,
                                     service_blacklist))
                {
                    start_new_introspect(system_bus, io, interface_map,
                                         process_name, assocMaps,
#ifdef DEBUG
                                         global_start_time,
#endif
                                         objectServer);
                    update_owners(system_bus, name_owners, process_name);
                }
            }
        },
        "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
        "ListNames");
}

void splitArgs(const std::string& stringArgs,
               boost::container::flat_set<std::string>& listArgs)
{
    std::istringstream args;
    std::string arg;

    args.str(stringArgs);

    while (!args.eof())
    {
        args >> arg;
        if (!arg.empty())
        {
            listArgs.insert(arg);
        }
    }
}

void addObjectMapResult(
    std::vector<interface_map_type::value_type>& objectMap,
    const std::string& objectPath,
    const std::pair<std::string, boost::container::flat_set<std::string>>&
        interfaceMap)
{
    // Adds an object path/service name/interface list entry to
    // the results of GetSubTree and GetAncestors.
    // If an entry for the object path already exists, just add the
    // service name and interfaces to that entry, otherwise create
    // a new entry.
    auto entry = std::find_if(
        objectMap.begin(), objectMap.end(),
        [&objectPath](const auto& i) { return objectPath == i.first; });

    if (entry != objectMap.end())
    {
        entry->second.emplace(interfaceMap);
    }
    else
    {
        interface_map_type::value_type object;
        object.first = objectPath;
        object.second.emplace(interfaceMap);
        objectMap.push_back(object);
    }
}

// Remove parents of the passed in path that:
// 1) Only have the 3 default interfaces on them
//    - Means D-Bus created these, not application code,
//      with the Properties, Introspectable, and Peer ifaces
// 2) Have no other child for this owner
void removeUnneededParents(const std::string& objectPath,
                           const std::string& owner,
                           interface_map_type& interface_map)
{
    auto parent = objectPath;

    while (true)
    {
        auto pos = parent.find_last_of('/');
        if ((pos == std::string::npos) || (pos == 0))
        {
            break;
        }
        parent = parent.substr(0, pos);

        auto parent_it = interface_map.find(parent);
        if (parent_it == interface_map.end())
        {
            break;
        }

        auto ifaces_it = parent_it->second.find(owner);
        if (ifaces_it == parent_it->second.end())
        {
            break;
        }

        if (ifaces_it->second.size() != 3)
        {
            break;
        }

        auto child_path = parent + '/';

        // Remove this parent if there isn't a remaining child on this owner
        auto child = std::find_if(
            interface_map.begin(), interface_map.end(),
            [&owner, &child_path](const auto& entry) {
                return boost::starts_with(entry.first, child_path) &&
                       (entry.second.find(owner) != entry.second.end());
            });

        if (child == interface_map.end())
        {
            parent_it->second.erase(ifaces_it);
            if (parent_it->second.empty())
            {
                interface_map.erase(parent_it);
            }
        }
        else
        {
            break;
        }
    }
}

int main(int argc, char** argv)
{
    auto options = ArgumentParser(argc, argv);
    boost::asio::io_service io;
    std::shared_ptr<sdbusplus::asio::connection> system_bus =
        std::make_shared<sdbusplus::asio::connection>(io);

    splitArgs(options["service-namespaces"], service_whitelist);
    splitArgs(options["service-blacklists"], service_blacklist);

    // TODO(Ed) Remove this once all service files are updated to not use this.
    // For now, simply squash the input, and ignore it.
    boost::container::flat_set<std::string> iface_whitelist;
    splitArgs(options["interface-namespaces"], iface_whitelist);

    system_bus->request_name(MAPPER_BUSNAME);
    sdbusplus::asio::object_server server(system_bus);

    // Construct a signal set registered for process termination.
    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&io](const boost::system::error_code& error,
                             int signal_number) { io.stop(); });

    interface_map_type interface_map;
    boost::container::flat_map<std::string, std::string> name_owners;

    std::function<void(sdbusplus::message::message & message)>
        nameChangeHandler = [&interface_map, &io, &name_owners, &server,
                             system_bus](sdbusplus::message::message& message) {
            std::string name;      // well-known
            std::string old_owner; // unique-name
            std::string new_owner; // unique-name

            message.read(name, old_owner, new_owner);

            if (!old_owner.empty())
            {
                processNameChangeDelete(name_owners, name, old_owner,
                                        interface_map, associationMaps, server);
            }

            if (!new_owner.empty())
            {
#ifdef DEBUG
                auto transaction = std::make_shared<
                    std::chrono::time_point<std::chrono::steady_clock>>(
                    std::chrono::steady_clock::now());
#endif
                // New daemon added
                if (needToIntrospect(name, service_whitelist,
                                     service_blacklist))
                {
                    name_owners[new_owner] = name;
                    start_new_introspect(system_bus.get(), io, interface_map,
                                         name, associationMaps,
#ifdef DEBUG
                                         transaction,
#endif
                                         server);
                }
            }
        };

    sdbusplus::bus::match::match nameOwnerChanged(
        static_cast<sdbusplus::bus::bus&>(*system_bus),
        sdbusplus::bus::match::rules::nameOwnerChanged(), nameChangeHandler);

    std::function<void(sdbusplus::message::message & message)>
        interfacesAddedHandler = [&interface_map, &name_owners, &server](
                                     sdbusplus::message::message& message) {
            sdbusplus::message::object_path obj_path;
            InterfacesAdded interfaces_added;
            message.read(obj_path, interfaces_added);
            std::string well_known;
            if (!getWellKnown(name_owners, message.get_sender(), well_known))
            {
                return; // only introspect well-known
            }
            if (needToIntrospect(well_known, service_whitelist,
                                 service_blacklist))
            {
                processInterfaceAdded(interface_map, obj_path, interfaces_added,
                                      well_known, associationMaps, server);
            }
        };

    sdbusplus::bus::match::match interfacesAdded(
        static_cast<sdbusplus::bus::bus&>(*system_bus),
        sdbusplus::bus::match::rules::interfacesAdded(),
        interfacesAddedHandler);

    std::function<void(sdbusplus::message::message & message)>
        interfacesRemovedHandler = [&interface_map, &name_owners, &server](
                                       sdbusplus::message::message& message) {
            sdbusplus::message::object_path obj_path;
            std::vector<std::string> interfaces_removed;
            message.read(obj_path, interfaces_removed);
            auto connection_map = interface_map.find(obj_path.str);
            if (connection_map == interface_map.end())
            {
                return;
            }

            std::string sender;
            if (!getWellKnown(name_owners, message.get_sender(), sender))
            {
                return;
            }
            for (const std::string& interface : interfaces_removed)
            {
                auto interface_set = connection_map->second.find(sender);
                if (interface_set == connection_map->second.end())
                {
                    continue;
                }

                if (interface == assocDefsInterface)
                {
                    removeAssociation(obj_path.str, sender, server,
                                      associationMaps);
                }

                interface_set->second.erase(interface);

                if (interface_set->second.empty())
                {
                    // If this was the last interface on this connection,
                    // erase the connection
                    connection_map->second.erase(interface_set);

                    // Instead of checking if every single path is the endpoint
                    // of an association that needs to be moved to pending,
                    // only check when the only remaining owner of this path is
                    // ourself, which would be because we still own the
                    // association path.
                    if ((connection_map->second.size() == 1) &&
                        (connection_map->second.begin()->first ==
                         MAPPER_BUSNAME))
                    {
                        // Remove the 2 association D-Bus paths and move the
                        // association to pending.
                        moveAssociationToPending(obj_path.str, associationMaps,
                                                 server);
                    }
                }
            }
            // If this was the last connection on this object path,
            // erase the object path
            if (connection_map->second.empty())
            {
                interface_map.erase(connection_map);
            }

            removeUnneededParents(obj_path.str, sender, interface_map);
        };

    sdbusplus::bus::match::match interfacesRemoved(
        static_cast<sdbusplus::bus::bus&>(*system_bus),
        sdbusplus::bus::match::rules::interfacesRemoved(),
        interfacesRemovedHandler);

    std::function<void(sdbusplus::message::message & message)>
        associationChangedHandler = [&server, &name_owners, &interface_map](
                                        sdbusplus::message::message& message) {
            std::string objectName;
            boost::container::flat_map<std::string,
                                       std::variant<std::vector<Association>>>
                values;
            message.read(objectName, values);
            auto prop = values.find(assocDefsProperty);
            if (prop != values.end())
            {
                std::vector<Association> associations =
                    std::get<std::vector<Association>>(prop->second);

                std::string well_known;
                if (!getWellKnown(name_owners, message.get_sender(),
                                  well_known))
                {
                    return;
                }
                associationChanged(server, associations, message.get_path(),
                                   well_known, interface_map, associationMaps);
            }
        };
    sdbusplus::bus::match::match assocChangedMatch(
        static_cast<sdbusplus::bus::bus&>(*system_bus),
        sdbusplus::bus::match::rules::interface(
            "org.freedesktop.DBus.Properties") +
            sdbusplus::bus::match::rules::member("PropertiesChanged") +
            sdbusplus::bus::match::rules::argN(0, assocDefsInterface),
        associationChangedHandler);

    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface(MAPPER_PATH, MAPPER_INTERFACE);

    iface->register_method(
        "GetAncestors", [&interface_map](std::string& req_path,
                                         std::vector<std::string>& interfaces) {
            // Interfaces need to be sorted for intersect to function
            std::sort(interfaces.begin(), interfaces.end());

            if (boost::ends_with(req_path, "/"))
            {
                req_path.pop_back();
            }
            if (req_path.size() &&
                interface_map.find(req_path) == interface_map.end())
            {
                throw NotFoundException();
            }

            std::vector<interface_map_type::value_type> ret;
            for (auto& object_path : interface_map)
            {
                auto& this_path = object_path.first;
                if (boost::starts_with(req_path, this_path) &&
                    (req_path != this_path))
                {
                    if (interfaces.empty())
                    {
                        ret.emplace_back(object_path);
                    }
                    else
                    {
                        for (auto& interface_map : object_path.second)
                        {

                            if (intersect(interfaces.begin(), interfaces.end(),
                                          interface_map.second.begin(),
                                          interface_map.second.end()))
                            {
                                addObjectMapResult(ret, this_path,
                                                   interface_map);
                            }
                        }
                    }
                }
            }

            return ret;
        });

    iface->register_method(
        "GetObject", [&interface_map](const std::string& path,
                                      std::vector<std::string>& interfaces) {
            boost::container::flat_map<std::string,
                                       boost::container::flat_set<std::string>>
                results;

            // Interfaces need to be sorted for intersect to function
            std::sort(interfaces.begin(), interfaces.end());
            auto path_ref = interface_map.find(path);
            if (path_ref == interface_map.end())
            {
                throw NotFoundException();
            }
            if (interfaces.empty())
            {
                return path_ref->second;
            }
            for (auto& interface_map : path_ref->second)
            {
                if (intersect(interfaces.begin(), interfaces.end(),
                              interface_map.second.begin(),
                              interface_map.second.end()))
                {
                    results.emplace(interface_map.first, interface_map.second);
                }
            }

            if (results.empty())
            {
                throw NotFoundException();
            }

            return results;
        });

    iface->register_method(
        "GetSubTree", [&interface_map](std::string& req_path, int32_t depth,
                                       std::vector<std::string>& interfaces) {
            if (depth <= 0)
            {
                depth = std::numeric_limits<int32_t>::max();
            }
            // Interfaces need to be sorted for intersect to function
            std::sort(interfaces.begin(), interfaces.end());
            std::vector<interface_map_type::value_type> ret;

            if (boost::ends_with(req_path, "/"))
            {
                req_path.pop_back();
            }
            if (req_path.size() &&
                interface_map.find(req_path) == interface_map.end())
            {
                throw NotFoundException();
            }

            for (auto& object_path : interface_map)
            {
                auto& this_path = object_path.first;

                if (this_path == req_path)
                {
                    continue;
                }

                if (boost::starts_with(this_path, req_path))
                {
                    // count the number of slashes past the search term
                    int32_t this_depth =
                        std::count(this_path.begin() + req_path.size(),
                                   this_path.end(), '/');
                    if (this_depth <= depth)
                    {
                        for (auto& interface_map : object_path.second)
                        {
                            if (intersect(interfaces.begin(), interfaces.end(),
                                          interface_map.second.begin(),
                                          interface_map.second.end()) ||
                                interfaces.empty())
                            {
                                addObjectMapResult(ret, this_path,
                                                   interface_map);
                            }
                        }
                    }
                }
            }

            return ret;
        });

    iface->register_method(
        "GetSubTreePaths",
        [&interface_map](std::string& req_path, int32_t depth,
                         std::vector<std::string>& interfaces) {
            if (depth <= 0)
            {
                depth = std::numeric_limits<int32_t>::max();
            }
            // Interfaces need to be sorted for intersect to function
            std::sort(interfaces.begin(), interfaces.end());
            std::vector<std::string> ret;

            if (boost::ends_with(req_path, "/"))
            {
                req_path.pop_back();
            }
            if (req_path.size() &&
                interface_map.find(req_path) == interface_map.end())
            {
                throw NotFoundException();
            }

            for (auto& object_path : interface_map)
            {
                auto& this_path = object_path.first;

                if (this_path == req_path)
                {
                    continue;
                }

                if (boost::starts_with(this_path, req_path))
                {
                    // count the number of slashes past the search term
                    int this_depth =
                        std::count(this_path.begin() + req_path.size(),
                                   this_path.end(), '/');
                    if (this_depth <= depth)
                    {
                        bool add = interfaces.empty();
                        for (auto& interface_map : object_path.second)
                        {
                            if (intersect(interfaces.begin(), interfaces.end(),
                                          interface_map.second.begin(),
                                          interface_map.second.end()))
                            {
                                add = true;
                                break;
                            }
                        }
                        if (add)
                        {
                            // TODO(ed) this is a copy
                            ret.emplace_back(this_path);
                        }
                    }
                }
            }

            return ret;
        });

    iface->initialize();

    io.post([&]() {
        doListNames(io, interface_map, system_bus.get(), name_owners,
                    associationMaps, server);
    });

    io.run();
}
