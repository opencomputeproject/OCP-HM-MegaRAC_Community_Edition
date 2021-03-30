#include "item_updater.hpp"
#include "mocked_utils.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace phosphor::software::updater;
using ::testing::_;
using ::testing::ContainerEq;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::ReturnArg;
using ::testing::StrEq;

using std::experimental::any;

class TestItemUpdater : public ::testing::Test
{
  public:
    using Properties = ItemUpdater::Properties;
    using PropertyType = utils::UtilsInterface::PropertyType;

    TestItemUpdater() :
        mockedUtils(
            reinterpret_cast<const utils::MockedUtils&>(utils::getUtils()))
    {
        ON_CALL(mockedUtils, getVersionId(_)).WillByDefault(ReturnArg<0>());
        ON_CALL(mockedUtils, getPropertyImpl(_, _, _, _, StrEq(PRESENT)))
            .WillByDefault(Return(any(PropertyType(true))));
    }

    ~TestItemUpdater()
    {
        utils::freeUtils();
    }

    auto& GetActivations()
    {
        return itemUpdater->activations;
    }

    std::string getObjPath(const std::string& versionId)
    {
        return std::string(dBusPath) + "/" + versionId;
    }

    void onPsuInventoryChanged(const std::string& psuPath,
                               const Properties& properties)
    {
        itemUpdater->onPsuInventoryChanged(psuPath, properties);
    }

    void scanDirectory(const fs::path& p)
    {
        itemUpdater->scanDirectory(p);
    }

    static constexpr auto dBusPath = SOFTWARE_OBJPATH;
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus::bus mockedBus = sdbusplus::get_mocked_new(&sdbusMock);
    const utils::MockedUtils& mockedUtils;
    std::unique_ptr<ItemUpdater> itemUpdater;
    Properties propAdded{{PRESENT, PropertyType(true)}};
    Properties propRemoved{{PRESENT, PropertyType(false)}};
    Properties propModel{{MODEL, PropertyType(std::string("dummyModel"))}};
};

TEST_F(TestItemUpdater, ctordtor)
{
    EXPECT_CALL(mockedUtils, getLatestVersion(_)).Times(1);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);
}

TEST_F(TestItemUpdater, NotCreateObjectOnNotPresent)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(false)))); // not present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // No activation/version objects are created
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);
}

TEST_F(TestItemUpdater, CreateOnePSUOnPresent)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // activation and version object will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);
}

TEST_F(TestItemUpdater, CreateTwoPSUsWithSameVersion)
{
    constexpr auto psu0 = "/com/example/inventory/psu0";
    constexpr auto psu1 = "/com/example/inventory/psu1";
    constexpr auto service = "com.example.Software.Psu";
    auto version0 = std::string("version0");
    auto version1 = std::string("version0");
    auto objPath0 = getObjPath(version0);
    auto objPath1 = getObjPath(version1);

    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psu0, psu1})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu0), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu1), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu0)))
        .WillOnce(Return(std::string(version0)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu0), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu1)))
        .WillOnce(Return(std::string(version1)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu1), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // activation and version object will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath0)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // Verify there is only one activation and it has two associations
    const auto& activations = GetActivations();
    EXPECT_EQ(1u, activations.size());
    const auto& activation = activations.find(version0)->second;
    const auto& assocs = activation->associations();
    EXPECT_EQ(2u, assocs.size());
    EXPECT_EQ(psu0, std::get<2>(assocs[0]));
    EXPECT_EQ(psu1, std::get<2>(assocs[1]));
}

TEST_F(TestItemUpdater, CreateTwoPSUsWithDifferentVersion)
{
    constexpr auto psu0 = "/com/example/inventory/psu0";
    constexpr auto psu1 = "/com/example/inventory/psu1";
    constexpr auto service = "com.example.Software.Psu";
    auto version0 = std::string("version0");
    auto version1 = std::string("version1");
    auto objPath0 = getObjPath(version0);
    auto objPath1 = getObjPath(version1);

    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psu0, psu1})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu0), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu1), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu0)))
        .WillOnce(Return(std::string(version0)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu0), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu1)))
        .WillOnce(Return(std::string(version1)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu1), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // two new activation and version objects will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath0)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath1)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // Verify there are two activations and each with one association
    const auto& activations = GetActivations();
    EXPECT_EQ(2u, activations.size());
    const auto& activation0 = activations.find(version0)->second;
    const auto& assocs0 = activation0->associations();
    EXPECT_EQ(1u, assocs0.size());
    EXPECT_EQ(psu0, std::get<2>(assocs0[0]));

    const auto& activation1 = activations.find(version1)->second;
    const auto& assocs1 = activation1->associations();
    EXPECT_EQ(1u, assocs1.size());
    EXPECT_EQ(psu1, std::get<2>(assocs1[0]));
}

TEST_F(TestItemUpdater, OnOnePSURemoved)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // activation and version object will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // the activation and version object will be removed
    Properties p{{PRESENT, PropertyType(false)}};
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(2);
    onPsuInventoryChanged(psuPath, p);

    // on exit, item updater is removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(dBusPath)))
        .Times(1);
}

TEST_F(TestItemUpdater, OnOnePSUAdded)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(false)))); // not present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // No activation/version objects are created
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // The PSU is present and version is added in a single call
    Properties propAddedAndModel{
        {PRESENT, PropertyType(true)},
        {MODEL, PropertyType(std::string("testModel"))}};
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);
    onPsuInventoryChanged(psuPath, propAddedAndModel);
}

TEST_F(TestItemUpdater, OnOnePSURemovedAndAddedWithLatestVersion)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    ON_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillByDefault(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // activation and version object will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // the activation and version object will be removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(2);
    onPsuInventoryChanged(psuPath, propRemoved);

    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);

    // On PSU inserted, it shall check if it's the latest version
    std::set<std::string> expectedVersions = {version};
    EXPECT_CALL(mockedUtils, getLatestVersion(ContainerEq(expectedVersions)))
        .WillOnce(Return(version));
    EXPECT_CALL(mockedUtils, isAssociated(StrEq(psuPath), _))
        .WillOnce(Return(true));
    EXPECT_CALL(sdbusMock, sd_bus_message_new_method_call(_, _, _, _, _,
                                                          StrEq("StartUnit")))
        .Times(0);
    onPsuInventoryChanged(psuPath, propAdded);
    onPsuInventoryChanged(psuPath, propModel);

    // on exit, objects are removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(dBusPath)))
        .Times(1);
}

TEST_F(TestItemUpdater,
       TwoPSUsWithSameVersionRemovedAndAddedWithDifferntVersion)
{
    constexpr auto psu0 = "/com/example/inventory/psu0";
    constexpr auto psu1 = "/com/example/inventory/psu1";
    constexpr auto service = "com.example.Software.Psu";
    auto version0 = std::string("version0");
    auto version1 = std::string("version0");
    auto objPath0 = getObjPath(version0);
    auto objPath1 = getObjPath(version1);

    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psu0, psu1})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu0), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu1), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu0)))
        .WillOnce(Return(std::string(version0)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu0), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu1)))
        .WillOnce(Return(std::string(version1)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu1), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // activation and version object will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath0)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // Verify there is only one activation and it has two associations
    const auto& activations = GetActivations();
    EXPECT_EQ(1u, activations.size());
    const auto& activation = activations.find(version0)->second;
    auto assocs = activation->associations();
    EXPECT_EQ(2u, assocs.size());
    EXPECT_EQ(psu0, std::get<2>(assocs[0]));
    EXPECT_EQ(psu1, std::get<2>(assocs[1]));

    // PSU0 is removed, only associations shall be updated
    onPsuInventoryChanged(psu0, propRemoved);
    assocs = activation->associations();
    EXPECT_EQ(1u, assocs.size());
    EXPECT_EQ(psu1, std::get<2>(assocs[0]));

    // PSU1 is removed, the activation and version object shall be removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath0)))
        .Times(2);
    onPsuInventoryChanged(psu1, propRemoved);

    // Add PSU0 and PSU1 back, but PSU1 with a different version
    version1 = "version1";
    objPath1 = getObjPath(version1);
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu0)))
        .WillOnce(Return(std::string(version0)));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu1)))
        .WillOnce(Return(std::string(version1)));
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath0)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath1)))
        .Times(2);
    onPsuInventoryChanged(psu0, propAdded);
    onPsuInventoryChanged(psu1, propModel);
    onPsuInventoryChanged(psu1, propAdded);
    onPsuInventoryChanged(psu0, propModel);

    // on exit, objects are removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath0)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath1)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(dBusPath)))
        .Times(1);
}

TEST_F(TestItemUpdater, scanDirOnNoPSU)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(false)))); // not present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // No activation/version objects are created
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // The valid image in test/psu-images-one-valid-one-invalid/model-1/
    auto objPathValid = getObjPath("psu-test.v0.4");
    auto objPathInvalid = getObjPath("psu-test.v0.5");
    // activation and version object will be added on scan dir
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPathValid)))
        .Times(2);
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPathInvalid)))
        .Times(0);
    scanDirectory("./psu-images-one-valid-one-invalid");
}

TEST_F(TestItemUpdater, scanDirOnSamePSUVersion)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string objPath = getObjPath(version);
    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psuPath})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    // The item updater itself
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(dBusPath)))
        .Times(1);

    // activation and version object will be added
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(2);
    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // The valid image in test/psu-images-valid-version0/model-3/ has the same
    // version as the running PSU, so no objects will be created, but only the
    // path will be set to the version object
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock, sd_bus_emit_properties_changed_strv(
                               _, StrEq(objPath),
                               StrEq("xyz.openbmc_project.Common.FilePath"),
                               Pointee(StrEq("Path"))))
        .Times(1);
    scanDirectory("./psu-images-valid-version0");
}

TEST_F(TestItemUpdater, OnUpdateDoneOnTwoPSUsWithSameVersion)
{
    // Simulate there are two PSUs with same version, and updated to a new
    // version
    constexpr auto psu0 = "/com/example/inventory/psu0";
    constexpr auto psu1 = "/com/example/inventory/psu1";
    constexpr auto service = "com.example.Software.Psu";
    auto version0 = std::string("version0");
    auto version1 = std::string("version0");
    auto objPath0 = getObjPath(version0);
    auto objPath1 = getObjPath(version1);

    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psu0, psu1})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu0), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu1), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu0)))
        .WillOnce(Return(std::string(version0)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu0), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu1)))
        .WillOnce(Return(std::string(version1)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu1), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    std::string newVersionId = "NewVersionId";
    AssociationList associations;
    auto dummyActivation = std::make_unique<Activation>(
        mockedBus, dBusPath, newVersionId, "", Activation::Status::Active,
        associations, "", itemUpdater.get(), itemUpdater.get());

    // Now there is one activation and it has two associations
    auto& activations = GetActivations();
    activations.emplace(newVersionId, std::move(dummyActivation));
    auto& activation = activations.find(version0)->second;
    auto assocs = activation->associations();
    EXPECT_EQ(2u, assocs.size());
    EXPECT_EQ(psu0, std::get<2>(assocs[0]));
    EXPECT_EQ(psu1, std::get<2>(assocs[1]));

    EXPECT_CALL(mockedUtils, isAssociated(StrEq(psu0), _))
        .WillOnce(Return(true));
    itemUpdater->onUpdateDone(newVersionId, psu0);

    // Now the activation should have one assoiation
    assocs = activation->associations();
    EXPECT_EQ(1u, assocs.size());
    EXPECT_EQ(psu1, std::get<2>(assocs[0]));

    EXPECT_CALL(mockedUtils, isAssociated(StrEq(psu1), _))
        .WillOnce(Return(true));
    itemUpdater->onUpdateDone(newVersionId, psu1);

    // Now the activation shall be erased and only the dummy one is left
    EXPECT_EQ(1u, activations.size());
    EXPECT_NE(activations.find(newVersionId), activations.end());
}

TEST_F(TestItemUpdater, OnUpdateDoneOnTwoPSUsWithDifferentVersion)
{
    // Simulate there are two PSUs with different version, and updated to a new
    // version
    constexpr auto psu0 = "/com/example/inventory/psu0";
    constexpr auto psu1 = "/com/example/inventory/psu1";
    constexpr auto service = "com.example.Software.Psu";
    auto version0 = std::string("version0");
    auto version1 = std::string("version1");
    auto objPath0 = getObjPath(version0);
    auto objPath1 = getObjPath(version1);

    EXPECT_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillOnce(Return(std::vector<std::string>({psu0, psu1})));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu0), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getService(_, StrEq(psu1), _))
        .WillOnce(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu0)))
        .WillOnce(Return(std::string(version0)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu0), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psu1)))
        .WillOnce(Return(std::string(version1)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psu1), _,
                                             StrEq(PRESENT)))
        .WillOnce(Return(any(PropertyType(true)))); // present

    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    std::string newVersionId = "NewVersionId";
    AssociationList associations;
    auto dummyActivation = std::make_unique<Activation>(
        mockedBus, dBusPath, newVersionId, "", Activation::Status::Active,
        associations, "", itemUpdater.get(), itemUpdater.get());

    auto& activations = GetActivations();
    activations.emplace(newVersionId, std::move(dummyActivation));

    EXPECT_CALL(mockedUtils, isAssociated(StrEq(psu0), _))
        .WillOnce(Return(true));

    // After psu0 is done, two activations should be left
    itemUpdater->onUpdateDone(newVersionId, psu0);
    EXPECT_EQ(2u, activations.size());
    const auto& activation1 = activations.find(version1)->second;
    const auto& assocs1 = activation1->associations();
    EXPECT_EQ(1u, assocs1.size());
    EXPECT_EQ(psu1, std::get<2>(assocs1[0]));

    EXPECT_CALL(mockedUtils, isAssociated(StrEq(psu1), _))
        .WillOnce(Return(true));
    // After psu1 is done, only the dummy activation should be left
    itemUpdater->onUpdateDone(newVersionId, psu1);
    EXPECT_EQ(1u, activations.size());
    EXPECT_NE(activations.find(newVersionId), activations.end());
}

TEST_F(TestItemUpdater, OnOnePSURemovedAndAddedWithOldVersion)
{
    constexpr auto psuPath = "/com/example/inventory/psu0";
    constexpr auto service = "com.example.Software.Psu";
    constexpr auto version = "version0";
    std::string versionId =
        version; // In testing versionId is the same as version
    std::string objPath = getObjPath(version);
    ON_CALL(mockedUtils, getPSUInventoryPath(_))
        .WillByDefault(Return(std::vector<std::string>({psuPath})));
    ON_CALL(mockedUtils, getService(_, StrEq(psuPath), _))
        .WillByDefault(Return(service));
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(version)));
    ON_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath), _,
                                         StrEq(PRESENT)))
        .WillByDefault(Return(any(PropertyType(true)))); // present

    itemUpdater = std::make_unique<ItemUpdater>(mockedBus, dBusPath);

    // Add an association to simulate that it has image in BMC filesystem
    auto& activation = GetActivations().find(versionId)->second;
    auto assocs = activation->associations();
    assocs.emplace_back(ACTIVATION_FWD_ASSOCIATION, ACTIVATION_REV_ASSOCIATION,
                        "SomePath");
    activation->associations(assocs);
    activation->path("SomeFilePath");

    onPsuInventoryChanged(psuPath, propRemoved);

    // On PSU inserted, it checks and finds a newer version
    auto oldVersion = "old-version";
    EXPECT_CALL(mockedUtils, getVersion(StrEq(psuPath)))
        .WillOnce(Return(std::string(oldVersion)));
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(MANUFACTURER)))
        .WillOnce(
            Return(any(PropertyType(std::string(""))))); // Checking compatible
    EXPECT_CALL(mockedUtils, getPropertyImpl(_, StrEq(service), StrEq(psuPath),
                                             _, StrEq(MODEL)))
        .WillOnce(
            Return(any(PropertyType(std::string(""))))); // Checking compatible
    std::set<std::string> expectedVersions = {version, oldVersion};
    EXPECT_CALL(mockedUtils, getLatestVersion(ContainerEq(expectedVersions)))
        .WillOnce(Return(version));
    ON_CALL(mockedUtils, isAssociated(StrEq(psuPath), _))
        .WillByDefault(Return(false));
    EXPECT_CALL(sdbusMock, sd_bus_message_new_method_call(_, _, _, _, _,
                                                          StrEq("StartUnit")))
        .Times(3); // There are 3 systemd units are started, enable bmc reboot
                   // guard, start activation, and disable bmc reboot guard
    onPsuInventoryChanged(psuPath, propAdded);
    onPsuInventoryChanged(psuPath, propModel);
}
