#include "src/associations.hpp"

#include "src/test/util/asio_server_class.hpp"
#include "src/test/util/association_objects.hpp"
#include "src/test/util/debug_output.hpp"

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <gtest/gtest.h>

class TestAssociations : public AsioServerClassTest
{
};
sdbusplus::asio::object_server* TestAssociations::AsioServerClassTest::server =
    nullptr;

// Verify call when path is not in associated owners
TEST_F(TestAssociations, SourcePathNotInAssociations)
{
    EXPECT_NE(nullptr, server);
    std::string sourcePath = "/xyz/openbmc_project/no/association";
    AssociationMaps assocMaps;

    removeAssociation(sourcePath, DEFAULT_DBUS_SVC, *server, assocMaps);
}

// Verify call when owner is not in associated owners
TEST_F(TestAssociations, OwnerNotInAssociations)
{
    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();

    removeAssociation(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC, *server,
                      assocMaps);
}

// Verify call when path is not in associated interfaces
TEST_F(TestAssociations, PathNotInAssocInterfaces)
{
    AssociationMaps assocMaps;

    assocMaps.owners = createDefaultOwnerAssociation();

    removeAssociation(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC, *server,
                      assocMaps);

    EXPECT_TRUE(assocMaps.owners.empty());
}

// Verify call when path is in associated interfaces
TEST_F(TestAssociations, PathIsInAssociatedInterfaces)
{
    // Build up these objects so that an associated interface will match
    // with the associated owner being removed
    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    removeAssociation(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC, *server,
                      assocMaps);

    // Verify owner association was deleted
    EXPECT_TRUE(assocMaps.owners.empty());

    // Verify endpoint was deleted from interface association
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
    intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_REV_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
}

// Verify call when path is in associated interfaces, with extra endpoints
TEST_F(TestAssociations, PathIsInAssociatedInterfacesExtraEndpoints)
{
    // Build up these objects so that an associated interface will match
    // with the associated owner being removed
    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    // Add another endpoint to the assoc interfaces
    addEndpointToInterfaceAssociation(assocMaps.ifaces);

    removeAssociation(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC, *server,
                      assocMaps);

    // Verify owner association was deleted
    EXPECT_TRUE(assocMaps.owners.empty());

    // Verify all endpoints are deleted since source path was deleted
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
    intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_REV_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
}

// Verify no associations or endpoints removed when the change is identical
TEST_F(TestAssociations, checkAssociationEndpointRemovesNoEpRemove)
{

    AssociationPaths newAssocPaths = {
        {DEFAULT_FWD_PATH, {DEFAULT_ENDPOINT}},
        {DEFAULT_REV_PATH, {DEFAULT_SOURCE_PATH}}};

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    checkAssociationEndpointRemoves(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC,
                                    newAssocPaths, *server, assocMaps);

    // Verify endpoints were not deleted because they matche with what was
    // in the original
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 1);
    intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_REV_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 1);
}

// Verify endpoint is removed when assoc path is different
TEST_F(TestAssociations, checkAssociationEndpointRemovesEpRemoveApDiff)
{
    AssociationPaths newAssocPaths = {{"/different/path", {DEFAULT_ENDPOINT}}};

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    checkAssociationEndpointRemoves(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC,
                                    newAssocPaths, *server, assocMaps);

    // Verify initial endpoints were deleted because the new path
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
    intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_REV_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
}

// Verify endpoint is removed when endpoint is different
TEST_F(TestAssociations, checkAssociationEndpointRemovesEpRemoveEpChanged)
{
    AssociationPaths newAssocPaths = {
        {DEFAULT_FWD_PATH, {DEFAULT_ENDPOINT + "/different"}},
        {DEFAULT_REV_PATH, {DEFAULT_SOURCE_PATH + "/different"}}};

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    checkAssociationEndpointRemoves(DEFAULT_SOURCE_PATH, DEFAULT_DBUS_SVC,
                                    newAssocPaths, *server, assocMaps);

    // Verify initial endpoints were deleted because of different endpoints
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
    intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_REV_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
}

// Verify existing endpoint deleted when empty endpoint is provided
TEST_F(TestAssociations, associationChangedEmptyEndpoint)
{
    std::vector<Association> associations = {
        {"inventory_cee", "error_cee", ""}};
    interface_map_type interfaceMap;

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    // Empty endpoint will result in deletion of corresponding assocInterface
    associationChanged(*server, associations, DEFAULT_SOURCE_PATH,
                       DEFAULT_DBUS_SVC, interfaceMap, assocMaps);

    // Both of these should be 0 since we have an invalid endpoint
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);
    intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_REV_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);

    EXPECT_EQ(assocMaps.pending.size(), 0);
}

// Add a new association with endpoint
TEST_F(TestAssociations, associationChangedAddNewAssoc)
{
    std::vector<Association> associations = {
        {"abc", "def", "/xyz/openbmc_project/new/endpoint"}};

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    // Make it look like the assoc endpoints are on D-Bus
    interface_map_type interfaceMap = {
        {"/new/source/path", {{DEFAULT_DBUS_SVC, {"a"}}}},
        {"/xyz/openbmc_project/new/endpoint", {{DEFAULT_DBUS_SVC, {"a"}}}}};

    associationChanged(*server, associations, "/new/source/path",
                       DEFAULT_DBUS_SVC, interfaceMap, assocMaps);

    // Two source paths
    EXPECT_EQ(assocMaps.owners.size(), 2);

    // Four interfaces
    EXPECT_EQ(assocMaps.ifaces.size(), 4);

    // Nothing pending
    EXPECT_EQ(assocMaps.pending.size(), 0);

    // New endpoint so assocMaps.ifaces should be same size
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 1);
}

// Add a new association to empty objects
TEST_F(TestAssociations, associationChangedAddNewAssocEmptyObj)
{
    std::string sourcePath = "/logging/entry/1";
    std::string owner = "xyz.openbmc_project.Test";
    std::vector<Association> associations = {
        {"inventory_canaeo", "error_canaeo",
         "/xyz/openbmc_project/inventory/system/chassis"}};

    // Empty objects because this test will ensure assocOwners adds the
    // changed association and interface
    AssociationMaps assocMaps;

    // Make it look like the assoc endpoints are on D-Bus
    interface_map_type interfaceMap = createDefaultInterfaceMap();

    associationChanged(*server, associations, DEFAULT_SOURCE_PATH,
                       DEFAULT_DBUS_SVC, interfaceMap, assocMaps);

    // New associations so ensure it now contains a single entry
    EXPECT_EQ(assocMaps.owners.size(), 1);

    // Nothing pending
    EXPECT_EQ(assocMaps.pending.size(), 0);

    // Verify corresponding assoc paths each have one endpoint in assoc
    // interfaces and that those endpoints match
    auto singleOwner = assocMaps.owners[DEFAULT_SOURCE_PATH];
    auto singleIntf = singleOwner[DEFAULT_DBUS_SVC];
    for (auto i : singleIntf)
    {
        auto intfEndpoints = std::get<endpointsPos>(assocMaps.ifaces[i.first]);
        EXPECT_EQ(intfEndpoints.size(), 1);
        EXPECT_EQ(intfEndpoints[0], *i.second.begin());
    }
}

// Add a new association to same source path but with new owner
TEST_F(TestAssociations, associationChangedAddNewAssocNewOwner)
{
    std::string newOwner = "xyz.openbmc_project.Test2";
    std::vector<Association> associations = {
        {"inventory_canano", "error_canano",
         "/xyz/openbmc_project/inventory/system/chassis"}};

    // Make it look like the assoc endpoints are on D-Bus
    interface_map_type interfaceMap = createDefaultInterfaceMap();

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    associationChanged(*server, associations, DEFAULT_SOURCE_PATH, newOwner,
                       interfaceMap, assocMaps);

    // New endpoint so assocOwners should be same size
    EXPECT_EQ(assocMaps.owners.size(), 1);

    // Ensure only one endpoint under first path
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 1);

    // Ensure the 2 new association endpoints are under the new owner
    auto a = assocMaps.owners.find(DEFAULT_SOURCE_PATH);
    auto o = a->second.find(newOwner);
    EXPECT_EQ(o->second.size(), 2);

    // Nothing pending
    EXPECT_EQ(assocMaps.pending.size(), 0);
}

// Add a new association to existing interface path
TEST_F(TestAssociations, associationChangedAddNewAssocSameInterface)
{
    std::vector<Association> associations = {
        {"abc", "error", "/xyz/openbmc_project/inventory/system/chassis"}};

    // Make it look like the assoc endpoints are on D-Bus
    interface_map_type interfaceMap = createDefaultInterfaceMap();

    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    associationChanged(*server, associations, DEFAULT_SOURCE_PATH,
                       DEFAULT_DBUS_SVC, interfaceMap, assocMaps);

    // Should have 3 entries in AssociationInterfaces, one is just missing an
    // endpoint
    EXPECT_EQ(assocMaps.ifaces.size(), 3);

    // Change to existing interface so it will be removed here
    auto intfEndpoints =
        std::get<endpointsPos>(assocMaps.ifaces[DEFAULT_FWD_PATH]);
    EXPECT_EQ(intfEndpoints.size(), 0);

    // The new endpoint should exist though in it's place
    intfEndpoints = std::get<endpointsPos>(
        assocMaps.ifaces[DEFAULT_SOURCE_PATH + "/" + "abc"]);
    EXPECT_EQ(intfEndpoints.size(), 1);

    // Added to an existing owner path so still 1
    EXPECT_EQ(assocMaps.owners.size(), 1);

    EXPECT_EQ(assocMaps.pending.size(), 0);
}

// Add 2 pending associations
TEST_F(TestAssociations, addPendingAssocs)
{
    AssociationMaps assocMaps;

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory", DEFAULT_ENDPOINT,
                          "error", DEFAULT_DBUS_SVC, assocMaps);

    EXPECT_TRUE(assocMaps.ifaces.empty());
    EXPECT_TRUE(assocMaps.owners.empty());

    EXPECT_EQ(assocMaps.pending.size(), 1);

    addPendingAssociation("some/other/path", "inventory", DEFAULT_ENDPOINT,
                          "error", DEFAULT_DBUS_SVC, assocMaps);

    EXPECT_TRUE(assocMaps.ifaces.empty());
    EXPECT_TRUE(assocMaps.owners.empty());

    EXPECT_EQ(assocMaps.pending.size(), 2);
}

// Test adding a new endpoint to a pending association
TEST_F(TestAssociations, addPendingAssocsNewEndpoints)
{
    AssociationMaps assocMaps;

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory", DEFAULT_ENDPOINT,
                          "error", DEFAULT_DBUS_SVC, assocMaps);

    EXPECT_EQ(assocMaps.pending.size(), 1);

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory",
                          "some/other/endpoint", "error", DEFAULT_DBUS_SVC,
                          assocMaps);

    // Same pending path, so still just 1 entry
    EXPECT_EQ(assocMaps.pending.size(), 1);

    auto assoc = assocMaps.pending.find(DEFAULT_SOURCE_PATH);
    EXPECT_NE(assoc, assocMaps.pending.end());

    auto& endpoints = assoc->second;
    EXPECT_EQ(endpoints.size(), 2);
}

// Test adding a new owner to a pending association
TEST_F(TestAssociations, addPendingAssocsNewOwner)
{
    AssociationMaps assocMaps;

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory", DEFAULT_ENDPOINT,
                          "error", DEFAULT_DBUS_SVC, assocMaps);

    EXPECT_EQ(assocMaps.pending.size(), 1);

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory", DEFAULT_ENDPOINT,
                          "error", "new owner", assocMaps);

    EXPECT_EQ(assocMaps.pending.size(), 1);

    auto assoc = assocMaps.pending.find(DEFAULT_SOURCE_PATH);
    EXPECT_NE(assoc, assocMaps.pending.end());

    auto& endpoints = assoc->second;
    EXPECT_EQ(endpoints.size(), 2);
}

// Add a pending association inside associationChanged
TEST_F(TestAssociations, associationChangedPending)
{
    std::vector<Association> associations = {
        {"abc", "def", "/xyz/openbmc_project/new/endpoint"}};

    AssociationMaps assocMaps;
    interface_map_type interfaceMap;

    associationChanged(*server, associations, "/new/source/path",
                       DEFAULT_DBUS_SVC, interfaceMap, assocMaps);

    // No associations were actually added
    EXPECT_EQ(assocMaps.owners.size(), 0);
    EXPECT_EQ(assocMaps.ifaces.size(), 0);

    // 1 pending association
    EXPECT_EQ(assocMaps.pending.size(), 1);
}

// Test removing pending associations
TEST_F(TestAssociations, testRemoveFromPendingAssociations)
{
    AssociationMaps assocMaps;

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory", DEFAULT_ENDPOINT,
                          "error", DEFAULT_DBUS_SVC, assocMaps);

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory",
                          "some/other/endpoint", "error", DEFAULT_DBUS_SVC,
                          assocMaps);

    EXPECT_EQ(assocMaps.pending.size(), 1);

    removeFromPendingAssociations("some/other/endpoint", assocMaps);

    // Still 1 pending entry, but down to 1 endpoint
    EXPECT_EQ(assocMaps.pending.size(), 1);

    auto assoc = assocMaps.pending.find(DEFAULT_SOURCE_PATH);
    EXPECT_NE(assoc, assocMaps.pending.end());
    auto& endpoints = assoc->second;
    EXPECT_EQ(endpoints.size(), 1);

    // Now nothing pending
    removeFromPendingAssociations(DEFAULT_ENDPOINT, assocMaps);
    EXPECT_EQ(assocMaps.pending.size(), 0);
}

// Test moving a pending association to a real one
TEST_F(TestAssociations, checkIfPending)
{
    AssociationMaps assocMaps;
    interface_map_type interfaceMap = {
        {DEFAULT_SOURCE_PATH, {{DEFAULT_DBUS_SVC, {"a"}}}},
        {DEFAULT_ENDPOINT, {{DEFAULT_DBUS_SVC, {"b"}}}}};

    addPendingAssociation(DEFAULT_SOURCE_PATH, "inventory_cip",
                          DEFAULT_ENDPOINT, "error_cip", DEFAULT_DBUS_SVC,
                          assocMaps);
    EXPECT_EQ(assocMaps.pending.size(), 1);

    // Move the pending association to a real association
    checkIfPendingAssociation(DEFAULT_SOURCE_PATH, interfaceMap, assocMaps,
                              *server);

    EXPECT_TRUE(assocMaps.pending.empty());
    EXPECT_EQ(assocMaps.owners.size(), 1);
    EXPECT_EQ(assocMaps.ifaces.size(), 2);

    // This shouldn't do anything, since /new/path isn't pending
    checkIfPendingAssociation("/new/path", interfaceMap, assocMaps, *server);
    EXPECT_TRUE(assocMaps.pending.empty());
    EXPECT_EQ(assocMaps.owners.size(), 1);
    EXPECT_EQ(assocMaps.ifaces.size(), 2);
}

TEST_F(TestAssociations, findAssociations)
{
    std::vector<std::tuple<std::string, Association>> associationData;
    AssociationMaps assocMaps;

    assocMaps.owners = {
        {"pathA",
         {{"ownerA",
           {{"pathA/typeA", {"endpointA", "endpointB"}},
            {"endpointA/type0", {"pathA"}}}}}},

        {"pathJ",
         {{"ownerC",
           {{"pathJ/typeA", {"endpointF"}}, {"endpointF/type0", {"pathJ"}}}}}},

        {"pathX",
         {{"ownerB",
           {{"pathX/typeB", {"endpointA"}}, {"endpointA/type1", {"pathX"}}}}}}};

    findAssociations("endpointA", assocMaps, associationData);
    ASSERT_EQ(associationData.size(), 2);

    {
        auto ad = std::find_if(
            associationData.begin(), associationData.end(),
            [](const auto& ad) { return std::get<0>(ad) == "ownerA"; });
        ASSERT_NE(ad, associationData.end());

        auto& a = std::get<1>(*ad);
        ASSERT_EQ(std::get<0>(a), "type0");
        ASSERT_EQ(std::get<1>(a), "typeA");
        ASSERT_EQ(std::get<2>(a), "pathA");
    }
    {
        auto ad = std::find_if(
            associationData.begin(), associationData.end(),
            [](const auto& ad) { return std::get<0>(ad) == "ownerB"; });
        ASSERT_NE(ad, associationData.end());

        auto& a = std::get<1>(*ad);
        ASSERT_EQ(std::get<0>(a), "type1");
        ASSERT_EQ(std::get<1>(a), "typeB");
        ASSERT_EQ(std::get<2>(a), "pathX");
    }
}

TEST_F(TestAssociations, moveAssocToPendingNoOp)
{
    AssociationMaps assocMaps;

    // Not an association, so it shouldn't do anything
    moveAssociationToPending(DEFAULT_ENDPOINT, assocMaps, *server);

    EXPECT_TRUE(assocMaps.pending.empty());
    EXPECT_TRUE(assocMaps.owners.empty());
    EXPECT_TRUE(assocMaps.ifaces.empty());
}

TEST_F(TestAssociations, moveAssocToPending)
{
    AssociationMaps assocMaps;
    assocMaps.owners = createDefaultOwnerAssociation();
    assocMaps.ifaces = createDefaultInterfaceAssociation(server);

    moveAssociationToPending(DEFAULT_ENDPOINT, assocMaps, *server);

    // Check it's now pending
    EXPECT_EQ(assocMaps.pending.size(), 1);
    EXPECT_EQ(assocMaps.pending.begin()->first, DEFAULT_ENDPOINT);

    // No more assoc owners
    EXPECT_TRUE(assocMaps.owners.empty());

    // Check the association interfaces were removed
    {
        auto assocs = assocMaps.ifaces.find(DEFAULT_FWD_PATH);
        auto& iface = std::get<ifacePos>(assocs->second);
        auto& endpoints = std::get<endpointsPos>(assocs->second);

        EXPECT_EQ(iface.get(), nullptr);
        EXPECT_TRUE(endpoints.empty());
    }
    {
        auto assocs = assocMaps.ifaces.find(DEFAULT_REV_PATH);
        auto& iface = std::get<ifacePos>(assocs->second);
        auto& endpoints = std::get<endpointsPos>(assocs->second);

        EXPECT_EQ(iface.get(), nullptr);
        EXPECT_TRUE(endpoints.empty());
    }
}
