#include "libpldm/platform.h"

#include "libpldmresponder/pdr_utils.hpp"
#include "libpldmresponder/platform.hpp"
#include "mocked_utils.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gtest/gtest.h>

using namespace pldm::responder;
using namespace pldm::responder::platform;
using namespace pldm::responder::pdr;
using namespace pldm::responder::pdr_utils;

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

TEST(GeneratePDRByStateEffecter, testGoodJson)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    Handler handler(&mockedUtils, "./pdr_jsons/state_effecter/good",
                    "./event_jsons/good", inPDRRepo, nullptr, nullptr);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_STATE_EFFECTER_PDR);

    // 2 entries
    ASSERT_EQ(outRepo.getRecordCount(), 2);

    // Check first PDR
    pdr_utils::PdrEntry e;
    auto record1 = pdr::getRecordByHandle(outRepo, 1, e);
    ASSERT_NE(record1, nullptr);
    pldm_state_effecter_pdr* pdr =
        reinterpret_cast<pldm_state_effecter_pdr*>(e.data);

    ASSERT_EQ(pdr->hdr.record_handle, 1);
    ASSERT_EQ(pdr->hdr.version, 1);
    ASSERT_EQ(pdr->hdr.type, PLDM_STATE_EFFECTER_PDR);
    ASSERT_EQ(pdr->hdr.record_change_num, 0);
    ASSERT_EQ(pdr->hdr.length, 23);

    ASSERT_EQ(pdr->terminus_handle, 0);
    ASSERT_EQ(pdr->effecter_id, 1);
    ASSERT_EQ(pdr->entity_type, 33);
    ASSERT_EQ(pdr->entity_instance, 0);
    ASSERT_EQ(pdr->container_id, 0);
    ASSERT_EQ(pdr->effecter_semantic_id, 0);
    ASSERT_EQ(pdr->effecter_init, PLDM_NO_INIT);
    ASSERT_EQ(pdr->has_description_pdr, false);
    ASSERT_EQ(pdr->composite_effecter_count, 2);
    state_effecter_possible_states* states =
        reinterpret_cast<state_effecter_possible_states*>(pdr->possible_states);
    ASSERT_EQ(states->state_set_id, 196);
    ASSERT_EQ(states->possible_states_size, 1);
    bitfield8_t bf1{};
    bf1.byte = 2;
    ASSERT_EQ(states->states[0].byte, bf1.byte);

    const auto& [dbusMappings1, dbusValMaps1] =
        handler.getDbusObjMaps(pdr->effecter_id);
    ASSERT_EQ(dbusMappings1[0].objectPath, "/foo/bar");

    // Check second PDR
    auto record2 = pdr::getRecordByHandle(outRepo, 2, e);
    ASSERT_NE(record2, nullptr);
    pdr = reinterpret_cast<pldm_state_effecter_pdr*>(e.data);

    ASSERT_EQ(pdr->hdr.record_handle, 2);
    ASSERT_EQ(pdr->hdr.version, 1);
    ASSERT_EQ(pdr->hdr.type, PLDM_STATE_EFFECTER_PDR);
    ASSERT_EQ(pdr->hdr.record_change_num, 0);
    ASSERT_EQ(pdr->hdr.length, 24);

    ASSERT_EQ(pdr->terminus_handle, 0);
    ASSERT_EQ(pdr->effecter_id, 2);
    ASSERT_EQ(pdr->entity_type, 100);
    ASSERT_EQ(pdr->entity_instance, 0);
    ASSERT_EQ(pdr->container_id, 0);
    ASSERT_EQ(pdr->effecter_semantic_id, 0);
    ASSERT_EQ(pdr->effecter_init, PLDM_NO_INIT);
    ASSERT_EQ(pdr->has_description_pdr, false);
    ASSERT_EQ(pdr->composite_effecter_count, 2);
    states =
        reinterpret_cast<state_effecter_possible_states*>(pdr->possible_states);
    ASSERT_EQ(states->state_set_id, 197);
    ASSERT_EQ(states->possible_states_size, 1);
    bf1.byte = 2;
    ASSERT_EQ(states->states[0].byte, bf1.byte);
    states = reinterpret_cast<state_effecter_possible_states*>(
        pdr->possible_states + sizeof(state_effecter_possible_states));
    ASSERT_EQ(states->state_set_id, 198);
    ASSERT_EQ(states->possible_states_size, 2);
    bitfield8_t bf2[2];
    bf2[0].byte = 38;
    bf2[1].byte = 128;
    ASSERT_EQ(states->states[0].byte, bf2[0].byte);
    ASSERT_EQ(states->states[1].byte, bf2[1].byte);

    const auto& [dbusMappings2, dbusValMaps2] =
        handler.getDbusObjMaps(pdr->effecter_id);
    ASSERT_EQ(dbusMappings2[0].objectPath, "/foo/bar");
    ASSERT_EQ(dbusMappings2[1].objectPath, "/foo/bar");

    ASSERT_THROW(handler.getDbusObjMaps(0xDEAD), std::exception);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(GeneratePDRByNumericEffecter, testGoodJson)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    Handler handler(&mockedUtils, "./pdr_jsons/state_effecter/good", "",
                    inPDRRepo, nullptr, nullptr);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_NUMERIC_EFFECTER_PDR);

    // 1 entries
    ASSERT_EQ(outRepo.getRecordCount(), 1);

    // Check first PDR
    pdr_utils::PdrEntry e;
    auto record = pdr::getRecordByHandle(outRepo, 3, e);
    ASSERT_NE(record, nullptr);

    pldm_numeric_effecter_value_pdr* pdr =
        reinterpret_cast<pldm_numeric_effecter_value_pdr*>(e.data);
    EXPECT_EQ(pdr->hdr.record_handle, 3);
    EXPECT_EQ(pdr->hdr.version, 1);
    EXPECT_EQ(pdr->hdr.type, PLDM_NUMERIC_EFFECTER_PDR);
    EXPECT_EQ(pdr->hdr.record_change_num, 0);
    EXPECT_EQ(pdr->hdr.length,
              sizeof(pldm_numeric_effecter_value_pdr) - sizeof(pldm_pdr_hdr));

    EXPECT_EQ(pdr->effecter_id, 3);
    EXPECT_EQ(pdr->effecter_data_size, 4);

    const auto& [dbusMappings, dbusValMaps] =
        handler.getDbusObjMaps(pdr->effecter_id);
    EXPECT_EQ(dbusMappings[0].objectPath, "/foo/bar");
    EXPECT_EQ(dbusMappings[0].interface, "xyz.openbmc_project.Foo.Bar");
    EXPECT_EQ(dbusMappings[0].propertyName, "propertyName");
    EXPECT_EQ(dbusMappings[0].propertyType, "uint64_t");

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(GeneratePDR, testMalformedJson)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    auto outPDRRepo = pldm_pdr_init();
    Repo outRepo(outPDRRepo);
    Handler handler(&mockedUtils, "./pdr_jsons/state_effecter/good",
                    "./event_jsons/good", inPDRRepo, nullptr, nullptr);
    Repo inRepo(inPDRRepo);
    getRepoByType(inRepo, outRepo, PLDM_STATE_EFFECTER_PDR);

    ASSERT_EQ(outRepo.getRecordCount(), 2);
    ASSERT_THROW(pdr_utils::readJson("./pdr_jsons/state_effecter/malformed"),
                 std::exception);

    pldm_pdr_destroy(inPDRRepo);
    pldm_pdr_destroy(outPDRRepo);
}

TEST(findStateEffecterId, goodJson)
{
    MockdBusHandler mockedUtils;
    EXPECT_CALL(mockedUtils, getService(StrEq("/foo/bar"), _))
        .Times(5)
        .WillRepeatedly(Return("foo.bar"));

    auto inPDRRepo = pldm_pdr_init();
    Handler handler(&mockedUtils, "./pdr_jsons/state_effecter/good", "",
                    inPDRRepo, nullptr, nullptr);
    uint16_t entityType = 33;
    uint16_t entityInstance = 0;
    uint16_t containerId = 0;
    uint16_t stateSetId = 196;
    auto effecterId = findStateEffecterId(inPDRRepo, entityType, entityInstance,
                                          containerId, stateSetId, true);
    ASSERT_EQ(effecterId, 1);
    stateSetId = 300;
    effecterId = findStateEffecterId(inPDRRepo, entityType, entityInstance,
                                     containerId, stateSetId, true);
    ASSERT_EQ(effecterId, PLDM_INVALID_EFFECTER_ID);
    pldm_pdr_destroy(inPDRRepo);
}
