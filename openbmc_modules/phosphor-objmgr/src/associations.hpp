#pragma once

#include "types.hpp"

constexpr const char* XYZ_ASSOCIATION_INTERFACE =
    "xyz.openbmc_project.Association";

/** @brief Remove input association
 *
 * @param[in] sourcePath          - Path of the object that contains the
 *                                  org.openbmc.Associations
 * @param[in] owner               - The Dbus service having its associations
 *                                  removed
 * @param[in,out] server          - sdbus system object
 * @param[in,out] assocMaps       - The association maps
 *
 * @return Void, server, assocMaps updated if needed
 */
void removeAssociation(const std::string& sourcePath, const std::string& owner,
                       sdbusplus::asio::object_server& server,
                       AssociationMaps& assocMaps);

/** @brief Remove input paths from endpoints of an association
 *
 * If the last endpoint was removed, then remove the whole
 * association object, otherwise just set the property
 *
 * @param[in] objectServer        - sdbus system object
 * @param[in] assocPath           - Path of the object that contains the
 *                                  org.openbmc.Associations
 * @param[in] endpointsToRemove   - Endpoints to remove
 * @param[in,out] assocMaps       - The association maps
 *
 * @return Void, objectServer and assocMaps updated if needed
 */
void removeAssociationEndpoints(
    sdbusplus::asio::object_server& objectServer, const std::string& assocPath,
    const boost::container::flat_set<std::string>& endpointsToRemove,
    AssociationMaps& assocMaps);

/** @brief Check and remove any changed associations
 *
 * Based on the latest values of the org.openbmc.Associations.associations
 * property, passed in via the newAssociations param, check if any of the
 * paths in the xyz.openbmc_project.Association.endpoints D-Bus property
 * for that association need to be removed.  If the last path is removed
 * from the endpoints property, remove that whole association object from
 * D-Bus.
 *
 * @param[in] sourcePath         - Path of the object that contains the
 *                                 org.openbmc.Associations
 * @param[in] owner              - The Dbus service having it's associatons
 *                                 changed
 * @param[in] newAssociations    - New associations to look at for change
 * @param[in,out] objectServer   - sdbus system object
 * @param[in,out] assocMaps      - The association maps
 *
 * @return Void, objectServer and assocMaps updated if needed
 */
void checkAssociationEndpointRemoves(
    const std::string& sourcePath, const std::string& owner,
    const AssociationPaths& newAssociations,
    sdbusplus::asio::object_server& objectServer, AssociationMaps& assocMaps);

/** @brief Handle new or changed association interfaces
 *
 * Called when either a new org.openbmc.Associations interface was
 * created, or the associations property on that interface changed
 *
 * @param[in,out] objectServer    - sdbus system object
 * @param[in] associations        - New associations to look at for change
 * @param[in] path                - Path of the object that contains the
 *                                  org.openbmc.Associations
 * @param[in] owner               - The Dbus service having it's associatons
 *                                  changed
 * @param[in] interfaceMap        - The full interface map
 * @param[in,out] assocMaps       - The association maps
 *
 * @return Void, objectServer and assocMaps updated if needed
 */
void associationChanged(sdbusplus::asio::object_server& objectServer,
                        const std::vector<Association>& associations,
                        const std::string& path, const std::string& owner,
                        const interface_map_type& interfaceMap,
                        AssociationMaps& assocMaps);

/** @brief Add a pending associations entry
 *
 *  Used when a client wants to create an association between
 *  2 D-Bus endpoint paths, but one of the paths doesn't exist.
 *  When the path does show up in D-Bus, if there is a pending
 *  association then the real association objects can be created.
 *
 * @param[in] objectPath    - The D-Bus object path that should be an
 *                            association endpoint but doesn't exist
 *                            on D-Bus.
 * @param[in] type          - The association type.  Gets used in the final
 *                            association path of <objectPath>/<type>.
 * @param[in] endpointPath  - The D-Bus path on the other side
 *                            of the association. This path exists.
 * @param[in] endpointType  - The endpoint association type. Gets used
 *                            in the final association path of
 *                            <endpointPath>/<endpointType>.
 * @param[in] owner         - The service name that owns the association.
 * @param[in,out] assocMaps - The association maps
 */
void addPendingAssociation(const std::string& objectPath,
                           const std::string& type,
                           const std::string& endpointPath,
                           const std::string& endpointType,
                           const std::string& owner,
                           AssociationMaps& assocMaps);

/** @brief Removes an endpoint from the pending associations map
 *
 * If the last endpoint is removed, removes the whole entry
 *
 * @param[in] endpointPath  - the endpoint path to remove
 * @param[in,out] assocMaps - The association maps
 */
void removeFromPendingAssociations(const std::string& endpointPath,
                                   AssociationMaps& assocMaps);

/** @brief Adds a single association D-Bus object (<path>/<type>)
 *
 * @param[in,out] server    - sdbus system object
 * @param[in] assocPath     - The association D-Bus path
 * @param[in] endpoint      - The association's D-Bus endpoint path
 * @param[in] owner         - The owning D-Bus well known name
 * @param[in] ownerPath     - The D-Bus path hosting the Associations property
 * @param[in,out] assocMaps - The association maps
 */
void addSingleAssociation(sdbusplus::asio::object_server& server,
                          const std::string& assocPath,
                          const std::string& endpoint, const std::string& owner,
                          const std::string& ownerPath,
                          AssociationMaps& assocMaps);

/** @brief Create a real association out of a pending association
 *         if there is one for this path.
 *
 * If objectPath is now on D-Bus, and it is also in the pending associations
 * map, create the 2 real association objects and remove its pending
 * associations entry.  Used when a new path shows up in D-Bus.
 *
 * @param[in] objectPath    - the path to check
 * @param[in] interfaceMap  - The master interface map
 * @param[in,out] assocMaps - The association maps
 * @param[in,out] server    - sdbus system object
 */
void checkIfPendingAssociation(const std::string& objectPath,
                               const interface_map_type& interfaceMap,
                               AssociationMaps& assocMaps,
                               sdbusplus::asio::object_server& server);

/** @brief Find all associations in the association owners map with the
 *         specified endpoint path.
 *
 * @param[in] endpointPath     - the endpoint path to look for
 * @param[in] assocMaps        - The association maps
 * @param[out] associationData - A vector of {owner, Association} tuples
 *                               of all the associations with that endpoint.
 */
void findAssociations(const std::string& endpointPath,
                      AssociationMaps& assocMaps,
                      FindAssocResults& associationData);

/** @brief If endpointPath is in an association, move that association
 *         to pending and remove the association objects.
 *
 *  Called when a path is going off of D-Bus.  If this path is an
 *  association endpoint (the path that owns the association is still
 *  on D-Bus), then move the association it's involved in to pending.
 *
 * @param[in] endpointPath  - the D-Bus endpoint path to check
 * @param[in,out] assocMaps - The association maps
 * @param[in,out] server    - sdbus system object
 */
void moveAssociationToPending(const std::string& endpointPath,
                              AssociationMaps& assocMaps,
                              sdbusplus::asio::object_server& server);
