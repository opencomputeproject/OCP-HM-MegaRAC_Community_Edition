#include "associations.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

void removeAssociation(const std::string& sourcePath, const std::string& owner,
                       sdbusplus::asio::object_server& server,
                       AssociationMaps& assocMaps)
{
    // Use associationOwners to find the association paths and endpoints
    // that the passed in object path and service own.  Remove all of
    // these endpoints from the actual association D-Bus objects, and if
    // the endpoints property is then empty, the whole association object
    // can be removed.  Note there can be multiple services that own an
    // association, and also that sourcePath is the path of the object
    // that contains the org.openbmc.Associations interface and not the
    // association path itself.

    // Find the services that have associations for this object path
    auto owners = assocMaps.owners.find(sourcePath);
    if (owners == assocMaps.owners.end())
    {
        return;
    }

    // Find the association paths and endpoints owned by this object
    // path for this service.
    auto assocs = owners->second.find(owner);
    if (assocs == owners->second.end())
    {
        return;
    }

    for (const auto& [assocPath, endpointsToRemove] : assocs->second)
    {
        removeAssociationEndpoints(server, assocPath, endpointsToRemove,
                                   assocMaps);
    }

    // Remove the associationOwners entries for this owning path/service.
    owners->second.erase(assocs);
    if (owners->second.empty())
    {
        assocMaps.owners.erase(owners);
    }

    // If we were still waiting on the other side of this association to
    // show up, cancel that wait.
    removeFromPendingAssociations(sourcePath, assocMaps);
}

void removeAssociationEndpoints(
    sdbusplus::asio::object_server& objectServer, const std::string& assocPath,
    const boost::container::flat_set<std::string>& endpointsToRemove,
    AssociationMaps& assocMaps)
{
    auto assoc = assocMaps.ifaces.find(assocPath);
    if (assoc == assocMaps.ifaces.end())
    {
        return;
    }

    auto& endpointsInDBus = std::get<endpointsPos>(assoc->second);

    for (const auto& endpointToRemove : endpointsToRemove)
    {
        auto e = std::find(endpointsInDBus.begin(), endpointsInDBus.end(),
                           endpointToRemove);

        if (e != endpointsInDBus.end())
        {
            endpointsInDBus.erase(e);
        }
    }

    if (endpointsInDBus.empty())
    {
        objectServer.remove_interface(std::get<ifacePos>(assoc->second));
        std::get<ifacePos>(assoc->second) = nullptr;
        std::get<endpointsPos>(assoc->second).clear();
    }
    else
    {
        std::get<ifacePos>(assoc->second)
            ->set_property("endpoints", endpointsInDBus);
    }
}

void checkAssociationEndpointRemoves(
    const std::string& sourcePath, const std::string& owner,
    const AssociationPaths& newAssociations,
    sdbusplus::asio::object_server& objectServer, AssociationMaps& assocMaps)
{
    // Find the services that have associations on this path.
    auto originalOwners = assocMaps.owners.find(sourcePath);
    if (originalOwners == assocMaps.owners.end())
    {
        return;
    }

    // Find the associations for this service
    auto originalAssociations = originalOwners->second.find(owner);
    if (originalAssociations == originalOwners->second.end())
    {
        return;
    }

    // Compare the new endpoints versus the original endpoints, and
    // remove any of the original ones that aren't in the new list.
    for (const auto& [originalAssocPath, originalEndpoints] :
         originalAssociations->second)
    {
        // Check if this source even still has each association that
        // was there previously, and if not, remove all of its endpoints
        // from the D-Bus endpoints property which will cause the whole
        // association path to be removed if no endpoints remain.
        auto newEndpoints = newAssociations.find(originalAssocPath);
        if (newEndpoints == newAssociations.end())
        {
            removeAssociationEndpoints(objectServer, originalAssocPath,
                                       originalEndpoints, assocMaps);
        }
        else
        {
            // The association is still there.  Check if the endpoints
            // changed.
            boost::container::flat_set<std::string> toRemove;

            for (auto& originalEndpoint : originalEndpoints)
            {
                if (std::find(newEndpoints->second.begin(),
                              newEndpoints->second.end(),
                              originalEndpoint) == newEndpoints->second.end())
                {
                    toRemove.emplace(originalEndpoint);
                }
            }
            if (!toRemove.empty())
            {
                removeAssociationEndpoints(objectServer, originalAssocPath,
                                           toRemove, assocMaps);
            }
        }
    }
}

void addEndpointsToAssocIfaces(
    sdbusplus::asio::object_server& objectServer, const std::string& assocPath,
    const boost::container::flat_set<std::string>& endpointPaths,
    AssociationMaps& assocMaps)
{
    auto& iface = assocMaps.ifaces[assocPath];
    auto& i = std::get<ifacePos>(iface);
    auto& endpoints = std::get<endpointsPos>(iface);

    // Only add new endpoints
    for (auto& e : endpointPaths)
    {
        if (std::find(endpoints.begin(), endpoints.end(), e) == endpoints.end())
        {
            endpoints.push_back(e);
        }
    }

    // If the interface already exists, only need to update
    // the property value, otherwise create it
    if (i)
    {
        i->set_property("endpoints", endpoints);
    }
    else
    {
        i = objectServer.add_interface(assocPath, XYZ_ASSOCIATION_INTERFACE);
        i->register_property("endpoints", endpoints);
        i->initialize();
    }
}

void associationChanged(sdbusplus::asio::object_server& objectServer,
                        const std::vector<Association>& associations,
                        const std::string& path, const std::string& owner,
                        const interface_map_type& interfaceMap,
                        AssociationMaps& assocMaps)
{
    AssociationPaths objects;

    for (const Association& association : associations)
    {
        std::string forward;
        std::string reverse;
        std::string endpoint;
        std::tie(forward, reverse, endpoint) = association;

        if (endpoint.empty())
        {
            std::cerr << "Found invalid association on path " << path << "\n";
            continue;
        }

        // Can't create this association if the endpoint isn't on D-Bus.
        if (interfaceMap.find(endpoint) == interfaceMap.end())
        {
            addPendingAssociation(endpoint, reverse, path, forward, owner,
                                  assocMaps);
            continue;
        }

        if (forward.size())
        {
            objects[path + "/" + forward].emplace(endpoint);
        }
        if (reverse.size())
        {
            objects[endpoint + "/" + reverse].emplace(path);
        }
    }
    for (const auto& object : objects)
    {
        addEndpointsToAssocIfaces(objectServer, object.first, object.second,
                                  assocMaps);
    }

    // Check for endpoints being removed instead of added
    checkAssociationEndpointRemoves(path, owner, objects, objectServer,
                                    assocMaps);

    if (!objects.empty())
    {
        // Update associationOwners with the latest info
        auto a = assocMaps.owners.find(path);
        if (a != assocMaps.owners.end())
        {
            auto o = a->second.find(owner);
            if (o != a->second.end())
            {
                o->second = std::move(objects);
            }
            else
            {
                a->second.emplace(owner, std::move(objects));
            }
        }
        else
        {
            boost::container::flat_map<std::string, AssociationPaths> owners;
            owners.emplace(owner, std::move(objects));
            assocMaps.owners.emplace(path, owners);
        }
    }
}

void addPendingAssociation(const std::string& objectPath,
                           const std::string& type,
                           const std::string& endpointPath,
                           const std::string& endpointType,
                           const std::string& owner, AssociationMaps& assocMaps)
{
    Association assoc{type, endpointType, endpointPath};

    auto p = assocMaps.pending.find(objectPath);
    if (p == assocMaps.pending.end())
    {
        ExistingEndpoints ee;
        ee.emplace_back(owner, std::move(assoc));
        assocMaps.pending.emplace(objectPath, std::move(ee));
    }
    else
    {
        // Already waiting on this path for another association,
        // so just add this endpoint and owner.
        auto& endpoints = p->second;
        auto e =
            std::find_if(endpoints.begin(), endpoints.end(),
                         [&assoc, &owner](const auto& endpoint) {
                             return (std::get<ownerPos>(endpoint) == owner) &&
                                    (std::get<assocPos>(endpoint) == assoc);
                         });
        if (e == endpoints.end())
        {
            endpoints.emplace_back(owner, std::move(assoc));
        }
    }
}

void removeFromPendingAssociations(const std::string& endpointPath,
                                   AssociationMaps& assocMaps)
{
    auto assoc = assocMaps.pending.begin();
    while (assoc != assocMaps.pending.end())
    {
        auto endpoint = assoc->second.begin();
        while (endpoint != assoc->second.end())
        {
            auto& e = std::get<assocPos>(*endpoint);
            if (std::get<reversePathPos>(e) == endpointPath)
            {
                endpoint = assoc->second.erase(endpoint);
                continue;
            }

            endpoint++;
        }

        if (assoc->second.empty())
        {
            assoc = assocMaps.pending.erase(assoc);
            continue;
        }

        assoc++;
    }
}

void addSingleAssociation(sdbusplus::asio::object_server& server,
                          const std::string& assocPath,
                          const std::string& endpoint, const std::string& owner,
                          const std::string& ownerPath,
                          AssociationMaps& assocMaps)
{
    boost::container::flat_set<std::string> endpoints{endpoint};

    addEndpointsToAssocIfaces(server, assocPath, endpoints, assocMaps);

    AssociationPaths objects;
    boost::container::flat_set e{endpoint};
    objects.emplace(assocPath, e);

    auto a = assocMaps.owners.find(ownerPath);
    if (a != assocMaps.owners.end())
    {
        auto o = a->second.find(owner);
        if (o != a->second.end())
        {
            auto p = o->second.find(assocPath);
            if (p != o->second.end())
            {
                p->second.emplace(endpoint);
            }
            else
            {
                o->second.emplace(assocPath, e);
            }
        }
        else
        {
            a->second.emplace(owner, std::move(objects));
        }
    }
    else
    {
        boost::container::flat_map<std::string, AssociationPaths> owners;
        owners.emplace(owner, std::move(objects));
        assocMaps.owners.emplace(endpoint, owners);
    }
}

void checkIfPendingAssociation(const std::string& objectPath,
                               const interface_map_type& interfaceMap,
                               AssociationMaps& assocMaps,
                               sdbusplus::asio::object_server& server)
{
    auto pending = assocMaps.pending.find(objectPath);
    if (pending == assocMaps.pending.end())
    {
        return;
    }

    if (interfaceMap.find(objectPath) == interfaceMap.end())
    {
        return;
    }

    auto endpoint = pending->second.begin();

    while (endpoint != pending->second.end())
    {
        const auto& e = std::get<assocPos>(*endpoint);

        // Ensure the other side of the association still exists
        if (interfaceMap.find(std::get<reversePathPos>(e)) ==
            interfaceMap.end())
        {
            endpoint++;
            continue;
        }

        // Add both sides of the association:
        //  objectPath/forwardType and reversePath/reverseType
        //
        // The ownerPath is the reversePath - i.e. the endpoint that
        // is on D-Bus and owns the org.openbmc.Associations iface.
        //
        const auto& ownerPath = std::get<reversePathPos>(e);
        const auto& owner = std::get<ownerPos>(*endpoint);

        auto assocPath = objectPath + '/' + std::get<forwardTypePos>(e);
        auto endpointPath = ownerPath;

        addSingleAssociation(server, assocPath, endpointPath, owner, ownerPath,
                             assocMaps);

        // Now the reverse direction (still the same owner and ownerPath)
        assocPath = endpointPath + '/' + std::get<reverseTypePos>(e);
        endpointPath = objectPath;
        addSingleAssociation(server, assocPath, endpointPath, owner, ownerPath,
                             assocMaps);

        // Not pending anymore
        endpoint = pending->second.erase(endpoint);
    }

    if (pending->second.empty())
    {
        assocMaps.pending.erase(objectPath);
    }
}

void findAssociations(const std::string& endpointPath,
                      AssociationMaps& assocMaps,
                      FindAssocResults& associationData)
{
    for (const auto& [sourcePath, owners] : assocMaps.owners)
    {
        for (const auto& [owner, assocs] : owners)
        {
            for (const auto& [assocPath, endpoints] : assocs)
            {
                if (std::find(endpoints.begin(), endpoints.end(),
                              endpointPath) != endpoints.end())
                {
                    // assocPath is <path>/<type> which tells us what is on the
                    // other side of the association.
                    auto pos = assocPath.rfind('/');
                    auto otherPath = assocPath.substr(0, pos);
                    auto otherType = assocPath.substr(pos + 1);

                    // Now we need to find the endpointPath/<type> ->
                    // [otherPath] entry so that we can get the type for
                    // endpointPath's side of the assoc.  Do this by finding
                    // otherPath as an endpoint, and also checking for
                    // 'endpointPath/*' as the key.
                    auto a = std::find_if(
                        assocs.begin(), assocs.end(),
                        [&endpointPath, &otherPath](const auto& ap) {
                            const auto& endpoints = ap.second;
                            auto endpoint = std::find(
                                endpoints.begin(), endpoints.end(), otherPath);
                            if (endpoint != endpoints.end())
                            {
                                return boost::starts_with(ap.first,
                                                          endpointPath + '/');
                            }
                            return false;
                        });

                    if (a != assocs.end())
                    {
                        // Pull out the type from endpointPath/<type>
                        pos = a->first.rfind('/');
                        auto thisType = a->first.substr(pos + 1);

                        // Now we know the full association:
                        // endpointPath/thisType -> otherPath/otherType
                        Association association{thisType, otherType, otherPath};
                        associationData.emplace_back(owner, association);
                    }
                }
            }
        }
    }
}

/** @brief Remove an endpoint for a particular association from D-Bus.
 *
 * If the last endpoint is gone, remove the whole association interface,
 * otherwise just update the D-Bus endpoints property.
 *
 * @param[in] assocPath     - the association path
 * @param[in] endpointPath  - the endpoint path to find and remove
 * @param[in,out] assocMaps - the association maps
 * @param[in,out] server    - sdbus system object
 */
void removeAssociationIfacesEntry(const std::string& assocPath,
                                  const std::string& endpointPath,
                                  AssociationMaps& assocMaps,
                                  sdbusplus::asio::object_server& server)
{
    auto assoc = assocMaps.ifaces.find(assocPath);
    if (assoc != assocMaps.ifaces.end())
    {
        auto& endpoints = std::get<endpointsPos>(assoc->second);
        auto e = std::find(endpoints.begin(), endpoints.end(), endpointPath);
        if (e != endpoints.end())
        {
            endpoints.erase(e);

            if (endpoints.empty())
            {
                server.remove_interface(std::get<ifacePos>(assoc->second));
                std::get<ifacePos>(assoc->second) = nullptr;
            }
            else
            {
                std::get<ifacePos>(assoc->second)
                    ->set_property("endpoints", endpoints);
            }
        }
    }
}

/** @brief Remove an endpoint from the association owners map.
 *
 * For a specific association path and owner, remove the endpoint.
 * Remove all remaining artifacts of that endpoint in the owners map
 * based on what frees up after the erase.
 *
 * @param[in] assocPath     - the association object path
 * @param[in] endpointPath  - the endpoint object path
 * @param[in] owner         - the owner of the association
 * @param[in,out] assocMaps - the association maps
 * @param[in,out] server    - sdbus system object
 */
void removeAssociationOwnersEntry(const std::string& assocPath,
                                  const std::string& endpointPath,
                                  const std::string& owner,
                                  AssociationMaps& assocMaps,
                                  sdbusplus::asio::object_server& server)
{
    auto sources = assocMaps.owners.begin();
    while (sources != assocMaps.owners.end())
    {
        auto owners = sources->second.find(owner);
        if (owners != sources->second.end())
        {
            auto entry = owners->second.find(assocPath);
            if (entry != owners->second.end())
            {
                auto e = std::find(entry->second.begin(), entry->second.end(),
                                   endpointPath);
                if (e != entry->second.end())
                {
                    entry->second.erase(e);
                    if (entry->second.empty())
                    {
                        owners->second.erase(entry);
                    }
                }
            }

            if (owners->second.empty())
            {
                sources->second.erase(owners);
            }
        }

        if (sources->second.empty())
        {
            sources = assocMaps.owners.erase(sources);
            continue;
        }
        sources++;
    }
}

void moveAssociationToPending(const std::string& endpointPath,
                              AssociationMaps& assocMaps,
                              sdbusplus::asio::object_server& server)
{
    FindAssocResults associationData;

    // Check which associations this path is an endpoint of, and
    // then add them to the pending associations map and remove
    // the associations objects.
    findAssociations(endpointPath, assocMaps, associationData);

    for (const auto& [owner, association] : associationData)
    {
        const auto& forwardPath = endpointPath;
        const auto& forwardType = std::get<forwardTypePos>(association);
        const auto& reversePath = std::get<reversePathPos>(association);
        const auto& reverseType = std::get<reverseTypePos>(association);

        addPendingAssociation(forwardPath, forwardType, reversePath,
                              reverseType, owner, assocMaps);

        // Remove both sides of the association from assocMaps.ifaces
        removeAssociationIfacesEntry(forwardPath + '/' + forwardType,
                                     reversePath, assocMaps, server);
        removeAssociationIfacesEntry(reversePath + '/' + reverseType,
                                     forwardPath, assocMaps, server);

        // Remove both sides of the association from assocMaps.owners
        removeAssociationOwnersEntry(forwardPath + '/' + forwardType,
                                     reversePath, owner, assocMaps, server);
        removeAssociationOwnersEntry(reversePath + '/' + reverseType,
                                     forwardPath, owner, assocMaps, server);
    }
}
