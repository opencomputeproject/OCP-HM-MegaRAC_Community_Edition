#include <array>

#include "libpldm/pdr.h"
#include "libpldm/platform.h"

#include <gtest/gtest.h>

TEST(PDRAccess, testInit)
{
    auto repo = pldm_pdr_init();
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 0u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), 0u);
    pldm_pdr_destroy(repo);
}

TEST(PDRUpdate, testAdd)
{
    auto repo = pldm_pdr_init();

    std::array<uint8_t, 10> data{};
    auto handle = pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    EXPECT_EQ(handle, 1u);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), data.size());

    handle = pldm_pdr_add(repo, data.data(), data.size(), 0u, false);
    EXPECT_EQ(handle, 2u);
    handle = pldm_pdr_add(repo, data.data(), data.size(), 0u, false);
    EXPECT_EQ(handle, 3u);
    handle = pldm_pdr_add(repo, data.data(), data.size(), htole32(0xdeeddeedu),
                          false);
    EXPECT_EQ(handle, htole32(0xdeeddeed));
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), data.size() * 4u);

    pldm_pdr_destroy(repo);
}

TEST(PDRUpdate, testRemove)
{
    std::array<uint8_t, 10> data{};

    auto repo = pldm_pdr_init();
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 0u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 0u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 0u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 2u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 6u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 3u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 5u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 3u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 5u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 2u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 3u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 5u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 2u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 3u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 3u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 2u);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    pldm_pdr_destroy(repo);

    repo = pldm_pdr_init();
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    pldm_pdr_remove_remote_pdrs(repo);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 2u);
    auto handle = pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    EXPECT_EQ(handle, 3u);
    handle = pldm_pdr_add(repo, data.data(), data.size(), 0, true);
    EXPECT_EQ(handle, 4u);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    pldm_pdr_destroy(repo);
}

TEST(PDRAccess, testGet)
{
    auto repo = pldm_pdr_init();

    std::array<uint32_t, 10> in{100, 345, 3, 6, 89, 0, 11, 45, 23434, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in.data()), sizeof(in), 1,
                 false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in));
    uint32_t size{};
    uint32_t nextRecHdl{};
    uint8_t* outData = nullptr;
    auto hdl = pldm_pdr_find_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 0u);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;

    auto hdl2 = pldm_pdr_find_record(repo, 1, &outData, &size, &nextRecHdl);
    EXPECT_EQ(hdl, hdl2);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 0u);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;

    hdl = pldm_pdr_find_record(repo, htole32(0xdeaddead), &outData, &size,
                               &nextRecHdl);
    EXPECT_EQ(hdl, nullptr);
    EXPECT_EQ(size, 0u);
    EXPECT_EQ(nextRecHdl, 0u);
    EXPECT_EQ(outData, nullptr);
    outData = nullptr;

    std::array<uint32_t, 10> in2{1000, 3450, 30,  60,     890,
                                 0,    110,  450, 234034, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 2,
                 false);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 3,
                 false);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 4,
                 true);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in2) * 4);
    hdl = pldm_pdr_find_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 2u);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;
    hdl2 = pldm_pdr_find_record(repo, 1, &outData, &size, &nextRecHdl);
    EXPECT_EQ(hdl, hdl2);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 2u);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;
    hdl = pldm_pdr_find_record(repo, 2, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 3u);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_find_record(repo, 3, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(pldm_pdr_record_is_remote(hdl), false);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 4u);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_find_record(repo, 4, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(pldm_pdr_record_is_remote(hdl), true);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 0u);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;

    pldm_pdr_destroy(repo);
}

TEST(PDRAccess, testGetNext)
{
    auto repo = pldm_pdr_init();

    std::array<uint32_t, 10> in{100, 345, 3, 6, 89, 0, 11, 45, 23434, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in.data()), sizeof(in), 1,
                 false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in));
    uint32_t size{};
    uint32_t nextRecHdl{};
    uint8_t* outData = nullptr;
    auto hdl = pldm_pdr_find_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in));
    EXPECT_EQ(nextRecHdl, 0u);
    EXPECT_EQ(memcmp(outData, in.data(), sizeof(in)), 0);
    outData = nullptr;

    std::array<uint32_t, 10> in2{1000, 3450, 30,  60,     890,
                                 0,    110,  450, 234034, 123123};
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 2,
                 false);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 3,
                 false);
    pldm_pdr_add(repo, reinterpret_cast<uint8_t*>(in2.data()), sizeof(in2), 4,
                 false);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 4u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo), sizeof(in2) * 4);
    hdl = pldm_pdr_get_next_record(repo, hdl, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 3u);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_next_record(repo, hdl, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 4u);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;
    hdl = pldm_pdr_get_next_record(repo, hdl, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(in2));
    EXPECT_EQ(nextRecHdl, 0u);
    EXPECT_EQ(memcmp(outData, in2.data(), sizeof(in2)), 0);
    outData = nullptr;

    pldm_pdr_destroy(repo);
}

TEST(PDRAccess, testFindByType)
{
    auto repo = pldm_pdr_init();

    std::array<uint8_t, sizeof(pldm_pdr_hdr)> data{};
    pldm_pdr_hdr* hdr = reinterpret_cast<pldm_pdr_hdr*>(data.data());
    hdr->type = 1;
    auto first = pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    hdr->type = 2;
    auto second = pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    hdr->type = 3;
    auto third = pldm_pdr_add(repo, data.data(), data.size(), 0, false);
    hdr->type = 4;
    auto fourth = pldm_pdr_add(repo, data.data(), data.size(), 0, false);

    uint8_t* outData = nullptr;
    uint32_t size{};
    auto firstRec =
        pldm_pdr_find_record_by_type(repo, 1, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, firstRec), first);
    outData = nullptr;
    auto secondRec =
        pldm_pdr_find_record_by_type(repo, 2, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, secondRec), second);
    outData = nullptr;
    auto thirdRec =
        pldm_pdr_find_record_by_type(repo, 3, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, thirdRec), third);
    outData = nullptr;
    auto fourthRec =
        pldm_pdr_find_record_by_type(repo, 4, nullptr, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, fourthRec), fourth);
    outData = nullptr;
    auto fifthRec =
        pldm_pdr_find_record_by_type(repo, 5, nullptr, &outData, &size);
    EXPECT_EQ(fifthRec, nullptr);
    EXPECT_EQ(outData, nullptr);
    EXPECT_EQ(size, 0u);

    auto rec =
        pldm_pdr_find_record_by_type(repo, 3, secondRec, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, rec), third);
    outData = nullptr;
    rec = pldm_pdr_find_record_by_type(repo, 4, secondRec, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, rec), fourth);
    outData = nullptr;
    rec = pldm_pdr_find_record_by_type(repo, 2, firstRec, &outData, &size);
    EXPECT_EQ(pldm_pdr_get_record_handle(repo, rec), second);
    outData = nullptr;

    pldm_pdr_destroy(repo);
}

TEST(PDRUpdate, testAddFruRecordSet)
{
    auto repo = pldm_pdr_init();

    auto handle = pldm_pdr_add_fru_record_set(repo, 1, 10, 1, 0, 100);
    EXPECT_EQ(handle, 1u);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 1u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo),
              sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_fru_record_set));
    uint32_t size{};
    uint32_t nextRecHdl{};
    uint8_t* outData = nullptr;
    auto hdl = pldm_pdr_find_record(repo, 0, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_fru_record_set));
    EXPECT_EQ(nextRecHdl, 0u);
    pldm_pdr_hdr* hdr = reinterpret_cast<pldm_pdr_hdr*>(outData);
    EXPECT_EQ(hdr->version, 1u);
    EXPECT_EQ(hdr->type, PLDM_PDR_FRU_RECORD_SET);
    EXPECT_EQ(hdr->length, htole16(sizeof(pldm_pdr_fru_record_set)));
    EXPECT_EQ(hdr->record_handle, htole32(1));
    pldm_pdr_fru_record_set* fru = reinterpret_cast<pldm_pdr_fru_record_set*>(
        outData + sizeof(pldm_pdr_hdr));
    EXPECT_EQ(fru->terminus_handle, htole16(1));
    EXPECT_EQ(fru->fru_rsi, htole16(10));
    EXPECT_EQ(fru->entity_type, htole16(1));
    EXPECT_EQ(fru->entity_instance_num, htole16(0));
    EXPECT_EQ(fru->container_id, htole16(100));
    outData = nullptr;

    handle = pldm_pdr_add_fru_record_set(repo, 2, 11, 2, 1, 101);
    EXPECT_EQ(handle, 2u);
    EXPECT_EQ(pldm_pdr_get_record_count(repo), 2u);
    EXPECT_EQ(pldm_pdr_get_repo_size(repo),
              2 * (sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_fru_record_set)));
    hdl = pldm_pdr_find_record(repo, 2, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_fru_record_set));
    EXPECT_EQ(nextRecHdl, 0u);
    hdr = reinterpret_cast<pldm_pdr_hdr*>(outData);
    EXPECT_EQ(hdr->version, 1u);
    EXPECT_EQ(hdr->type, PLDM_PDR_FRU_RECORD_SET);
    EXPECT_EQ(hdr->length, htole16(sizeof(pldm_pdr_fru_record_set)));
    EXPECT_EQ(hdr->record_handle, htole32(2));
    fru = reinterpret_cast<pldm_pdr_fru_record_set*>(outData +
                                                     sizeof(pldm_pdr_hdr));
    EXPECT_EQ(fru->terminus_handle, htole16(2));
    EXPECT_EQ(fru->fru_rsi, htole16(11));
    EXPECT_EQ(fru->entity_type, htole16(2));
    EXPECT_EQ(fru->entity_instance_num, htole16(1));
    EXPECT_EQ(fru->container_id, htole16(101));
    outData = nullptr;

    hdl = pldm_pdr_find_record(repo, 1, &outData, &size, &nextRecHdl);
    EXPECT_NE(hdl, nullptr);
    EXPECT_EQ(size, sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_fru_record_set));
    EXPECT_EQ(nextRecHdl, 2u);
    hdr = reinterpret_cast<pldm_pdr_hdr*>(outData);
    EXPECT_EQ(hdr->version, 1u);
    EXPECT_EQ(hdr->type, PLDM_PDR_FRU_RECORD_SET);
    EXPECT_EQ(hdr->length, htole16(sizeof(pldm_pdr_fru_record_set)));
    EXPECT_EQ(hdr->record_handle, htole32(1));
    fru = reinterpret_cast<pldm_pdr_fru_record_set*>(outData +
                                                     sizeof(pldm_pdr_hdr));
    EXPECT_EQ(fru->terminus_handle, htole16(1));
    EXPECT_EQ(fru->fru_rsi, htole16(10));
    EXPECT_EQ(fru->entity_type, htole16(1));
    EXPECT_EQ(fru->entity_instance_num, htole16(0));
    EXPECT_EQ(fru->container_id, htole16(100));
    outData = nullptr;

    pldm_pdr_destroy(repo);
}

TEST(PDRUpdate, tesFindtFruRecordSet)
{
    auto repo = pldm_pdr_init();

    uint16_t terminusHdl{};
    uint16_t entityType{};
    uint16_t entityInstanceNum{};
    uint16_t containerId{};
    auto first = pldm_pdr_add_fru_record_set(repo, 1, 1, 1, 0, 100);
    auto second = pldm_pdr_add_fru_record_set(repo, 1, 2, 1, 1, 100);
    auto third = pldm_pdr_add_fru_record_set(repo, 1, 3, 1, 2, 100);
    EXPECT_EQ(first, pldm_pdr_get_record_handle(
                         repo, pldm_pdr_fru_record_set_find_by_rsi(
                                   repo, 1, &terminusHdl, &entityType,
                                   &entityInstanceNum, &containerId)));
    EXPECT_EQ(second, pldm_pdr_get_record_handle(
                          repo, pldm_pdr_fru_record_set_find_by_rsi(
                                    repo, 2, &terminusHdl, &entityType,
                                    &entityInstanceNum, &containerId)));
    EXPECT_EQ(third, pldm_pdr_get_record_handle(
                         repo, pldm_pdr_fru_record_set_find_by_rsi(
                                   repo, 3, &terminusHdl, &entityType,
                                   &entityInstanceNum, &containerId)));
    EXPECT_EQ(terminusHdl, 1u);
    EXPECT_EQ(entityType, 1u);
    EXPECT_EQ(entityInstanceNum, 2u);
    EXPECT_EQ(containerId, 100u);
    EXPECT_EQ(nullptr, pldm_pdr_fru_record_set_find_by_rsi(
                           repo, 4, &terminusHdl, &entityType,
                           &entityInstanceNum, &containerId));

    pldm_pdr_destroy(repo);
}

TEST(EntityAssociationPDR, testInit)
{
    auto tree = pldm_entity_association_tree_init();
    EXPECT_NE(tree, nullptr);
    pldm_entity_association_tree_destroy(tree);
}

TEST(EntityAssociationPDR, testBuild)
{
    //        1
    //        |
    //        2--3--4
    //        |
    //        5--6--7
    //        |  |
    //        9  8

    pldm_entity entities[9]{};

    entities[0].entity_type = 1;
    entities[1].entity_type = 2;
    entities[2].entity_type = 2;
    entities[3].entity_type = 3;
    entities[4].entity_type = 4;
    entities[5].entity_type = 5;
    entities[6].entity_type = 5;
    entities[7].entity_type = 6;
    entities[8].entity_type = 7;

    auto tree = pldm_entity_association_tree_init();

    auto l1 = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                               PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l1, nullptr);
    auto l2a = pldm_entity_association_tree_add(
        tree, &entities[1], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2a, nullptr);
    auto l2b = pldm_entity_association_tree_add(
        tree, &entities[2], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2b, nullptr);
    auto l2c = pldm_entity_association_tree_add(
        tree, &entities[3], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2c, nullptr);
    auto l3a = pldm_entity_association_tree_add(
        tree, &entities[4], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3a, nullptr);
    auto l3b = pldm_entity_association_tree_add(
        tree, &entities[5], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3b, nullptr);
    auto l3c = pldm_entity_association_tree_add(
        tree, &entities[6], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3b, nullptr);
    auto l4a = pldm_entity_association_tree_add(
        tree, &entities[7], l3a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l4a, nullptr);
    auto l4b = pldm_entity_association_tree_add(
        tree, &entities[8], l3b, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l4b, nullptr);

    EXPECT_EQ(pldm_entity_is_node_parent(l1), true);
    EXPECT_EQ(pldm_entity_is_node_parent(l2a), true);
    EXPECT_EQ(pldm_entity_is_node_parent(l3a), true);
    EXPECT_EQ(pldm_entity_is_node_parent(l3b), true);

    EXPECT_EQ(pldm_entity_is_node_parent(l2b), false);
    EXPECT_EQ(pldm_entity_is_node_parent(l2c), false);
    EXPECT_EQ(pldm_entity_is_node_parent(l3c), false);
    EXPECT_EQ(pldm_entity_is_node_parent(l4a), false);
    EXPECT_EQ(pldm_entity_is_node_parent(l4b), false);

    size_t num{};
    pldm_entity* out = nullptr;
    pldm_entity_association_tree_visit(tree, &out, &num);
    EXPECT_EQ(num, 9u);

    EXPECT_EQ(out[0].entity_type, 1u);
    EXPECT_EQ(out[0].entity_instance_num, 1u);
    EXPECT_EQ(out[0].entity_container_id, 0u);

    EXPECT_EQ(out[1].entity_type, 2u);
    EXPECT_EQ(out[1].entity_instance_num, 1u);
    EXPECT_EQ(out[1].entity_container_id, 1u);
    EXPECT_EQ(out[2].entity_type, 2u);
    EXPECT_EQ(out[2].entity_instance_num, 2u);
    EXPECT_EQ(out[2].entity_container_id, 1u);
    EXPECT_EQ(out[3].entity_type, 3u);
    EXPECT_EQ(out[3].entity_instance_num, 1u);
    EXPECT_EQ(out[3].entity_container_id, 1u);

    EXPECT_EQ(out[4].entity_type, 4u);
    EXPECT_EQ(out[4].entity_instance_num, 1u);
    EXPECT_EQ(out[4].entity_container_id, 2u);
    EXPECT_EQ(out[5].entity_type, 5u);
    EXPECT_EQ(out[5].entity_instance_num, 1u);
    EXPECT_EQ(out[5].entity_container_id, 2u);
    EXPECT_EQ(out[6].entity_type, 5u);
    EXPECT_EQ(out[6].entity_instance_num, 2u);
    EXPECT_EQ(out[6].entity_container_id, 2u);

    EXPECT_EQ(out[7].entity_type, 7u);
    EXPECT_EQ(out[7].entity_instance_num, 1u);
    EXPECT_EQ(out[7].entity_container_id, 4u);
    EXPECT_EQ(out[8].entity_type, 6u);
    EXPECT_EQ(out[8].entity_instance_num, 1u);
    EXPECT_EQ(out[8].entity_container_id, 3u);

    free(out);
    pldm_entity_association_tree_destroy(tree);
}

TEST(EntityAssociationPDR, testSpecialTrees)
{
    pldm_entity entities[3]{};

    entities[0].entity_type = 1;
    entities[1].entity_type = 2;
    entities[2].entity_type = 1;

    // A
    auto tree = pldm_entity_association_tree_init();
    auto node = pldm_entity_association_tree_add(
        tree, &entities[0], nullptr, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    size_t num{};
    pldm_entity* out = nullptr;
    pldm_entity_association_tree_visit(tree, &out, &num);
    EXPECT_EQ(num, 1u);
    EXPECT_EQ(out[0].entity_type, 1u);
    EXPECT_EQ(out[0].entity_instance_num, 1u);
    EXPECT_EQ(out[0].entity_container_id, 0u);
    free(out);
    pldm_entity_association_tree_destroy(tree);

    // A-A-A
    tree = pldm_entity_association_tree_init();
    node = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                            PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    node = pldm_entity_association_tree_add(tree, &entities[1], nullptr,
                                            PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    node = pldm_entity_association_tree_add(tree, &entities[2], nullptr,
                                            PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    pldm_entity_association_tree_visit(tree, &out, &num);
    EXPECT_EQ(num, 3u);
    EXPECT_EQ(out[0].entity_type, 1u);
    EXPECT_EQ(out[0].entity_instance_num, 1u);
    EXPECT_EQ(out[0].entity_container_id, 0u);
    EXPECT_EQ(out[1].entity_type, 1u);
    EXPECT_EQ(out[1].entity_instance_num, 2u);
    EXPECT_EQ(out[1].entity_container_id, 0u);
    EXPECT_EQ(out[2].entity_type, 2u);
    EXPECT_EQ(out[2].entity_instance_num, 1u);
    EXPECT_EQ(out[2].entity_container_id, 0u);
    free(out);
    pldm_entity_association_tree_destroy(tree);

    // A
    // |
    // A
    // |
    // A
    tree = pldm_entity_association_tree_init();
    node = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                            PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    auto node1 = pldm_entity_association_tree_add(
        tree, &entities[1], node, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node1, nullptr);
    auto node2 = pldm_entity_association_tree_add(
        tree, &entities[2], node1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node2, nullptr);
    pldm_entity_association_tree_visit(tree, &out, &num);
    EXPECT_EQ(num, 3u);
    EXPECT_EQ(out[0].entity_type, 1u);
    EXPECT_EQ(out[0].entity_instance_num, 1u);
    EXPECT_EQ(out[0].entity_container_id, 0u);
    EXPECT_EQ(out[1].entity_type, 2u);
    EXPECT_EQ(out[1].entity_instance_num, 1u);
    EXPECT_EQ(out[1].entity_container_id, 1u);
    EXPECT_EQ(out[2].entity_type, 1u);
    EXPECT_EQ(out[2].entity_instance_num, 1u);
    EXPECT_EQ(out[2].entity_container_id, 2u);
    free(out);
    pldm_entity_association_tree_destroy(tree);

    // A-A
    //   |
    //   A-A
    tree = pldm_entity_association_tree_init();
    node = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                            PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    node = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                            PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node, nullptr);
    node1 = pldm_entity_association_tree_add(tree, &entities[1], node,
                                             PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node1, nullptr);
    node2 = pldm_entity_association_tree_add(tree, &entities[2], node,
                                             PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(node2, nullptr);
    pldm_entity_association_tree_visit(tree, &out, &num);
    EXPECT_EQ(num, 4u);
    EXPECT_EQ(out[0].entity_type, 1u);
    EXPECT_EQ(out[0].entity_instance_num, 1u);
    EXPECT_EQ(out[0].entity_container_id, 0u);
    EXPECT_EQ(out[1].entity_type, 1u);
    EXPECT_EQ(out[1].entity_instance_num, 2u);
    EXPECT_EQ(out[1].entity_container_id, 0u);
    EXPECT_EQ(out[2].entity_type, 2u);
    EXPECT_EQ(out[2].entity_instance_num, 1u);
    EXPECT_EQ(out[2].entity_container_id, 1u);
    EXPECT_EQ(out[3].entity_type, 1u);
    EXPECT_EQ(out[3].entity_instance_num, 1u);
    EXPECT_EQ(out[3].entity_container_id, 1u);
    free(out);
    pldm_entity_association_tree_destroy(tree);
}

TEST(EntityAssociationPDR, testPDR)
{
    // e = entity type, c = container id, i = instance num

    //        INPUT
    //        1(e=1)--1a(e=2)
    //        |
    //        2(e=2)--3(e=2)--4(e=2)--5(e=3)
    //        |
    //        6(e=4)--7(e=5)--8(e=5)--9(e=5)
    //        |       |
    //        11(e=6) 10(e=7)

    //        Expected OUTPUT
    //        1(e=1,c=0,i=1)
    //        |
    //        2(e=2,c=1,i=1)--3(e=2,c=1,i=2)--4(e=3,c=1,i=1)--5(e=3,c=1,i=2)
    //        |
    //        6(e=4,c=2,i=1)--7(e=5,c=2,i=1)--8(e=5,c=2,i=2)--9(e=5,c=2,i=3)
    //        |               |
    //        10(e=6,c=3,i=1) 11(e=7,c=4,i=1)
    pldm_entity entities[11]{};

    entities[0].entity_type = 1;
    entities[1].entity_type = 2;
    entities[2].entity_type = 3;
    entities[3].entity_type = 2;
    entities[4].entity_type = 3;
    entities[5].entity_type = 4;
    entities[6].entity_type = 5;
    entities[7].entity_type = 5;
    entities[8].entity_type = 5;
    entities[9].entity_type = 6;
    entities[10].entity_type = 7;

    auto tree = pldm_entity_association_tree_init();

    auto l1 = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                               PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l1, nullptr);
    auto l1a = pldm_entity_association_tree_add(
        tree, &entities[1], nullptr, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l1a, nullptr);

    auto l2a = pldm_entity_association_tree_add(
        tree, &entities[1], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2a, nullptr);
    auto l2b = pldm_entity_association_tree_add(tree, &entities[2], l1,
                                                PLDM_ENTITY_ASSOCIAION_LOGICAL);
    EXPECT_NE(l2b, nullptr);
    auto l2c = pldm_entity_association_tree_add(
        tree, &entities[3], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2c, nullptr);
    auto l2d = pldm_entity_association_tree_add(tree, &entities[4], l1,
                                                PLDM_ENTITY_ASSOCIAION_LOGICAL);
    EXPECT_NE(l2d, nullptr);

    auto l3a = pldm_entity_association_tree_add(
        tree, &entities[5], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3a, nullptr);
    auto l3b = pldm_entity_association_tree_add(
        tree, &entities[6], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3b, nullptr);
    auto l3c = pldm_entity_association_tree_add(tree, &entities[7], l2a,
                                                PLDM_ENTITY_ASSOCIAION_LOGICAL);
    EXPECT_NE(l3c, nullptr);
    auto l3d = pldm_entity_association_tree_add(tree, &entities[8], l2a,
                                                PLDM_ENTITY_ASSOCIAION_LOGICAL);
    EXPECT_NE(l3d, nullptr);

    auto l4a = pldm_entity_association_tree_add(
        tree, &entities[9], l3a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l4a, nullptr);
    auto l4b = pldm_entity_association_tree_add(tree, &entities[10], l3b,
                                                PLDM_ENTITY_ASSOCIAION_LOGICAL);
    EXPECT_NE(l4b, nullptr);

    EXPECT_EQ(pldm_entity_get_num_children(l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL),
              2);
    EXPECT_EQ(pldm_entity_get_num_children(l1, PLDM_ENTITY_ASSOCIAION_LOGICAL),
              2);
    EXPECT_EQ(
        pldm_entity_get_num_children(l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL), 2);
    EXPECT_EQ(
        pldm_entity_get_num_children(l3b, PLDM_ENTITY_ASSOCIAION_PHYSICAL), 0);
    EXPECT_EQ(pldm_entity_get_num_children(l3b, PLDM_ENTITY_ASSOCIAION_LOGICAL),
              1);

    auto repo = pldm_pdr_init();
    pldm_entity_association_pdr_add(tree, repo, false);

    EXPECT_EQ(pldm_pdr_get_record_count(repo), 6u);

    uint32_t currRecHandle{};
    uint32_t nextRecHandle{};
    uint8_t* data = nullptr;
    uint32_t size{};
    uint32_t commonSize = sizeof(struct pldm_pdr_hdr) + sizeof(uint16_t) +
                          sizeof(uint8_t) + sizeof(pldm_entity) +
                          sizeof(uint8_t);

    pldm_pdr_find_record(repo, currRecHandle, &data, &size, &nextRecHandle);
    EXPECT_EQ(size, commonSize + (pldm_entity_get_num_children(
                                      l1, PLDM_ENTITY_ASSOCIAION_LOGICAL) *
                                  sizeof(pldm_entity)));
    uint8_t* start = data;
    pldm_pdr_hdr* hdr = reinterpret_cast<pldm_pdr_hdr*>(start);
    EXPECT_EQ(le32toh(hdr->record_handle), 1u);
    EXPECT_EQ(hdr->type, PLDM_PDR_ENTITY_ASSOCIATION);
    EXPECT_EQ(le16toh(hdr->length), size - sizeof(struct pldm_pdr_hdr));
    start += sizeof(pldm_pdr_hdr);
    uint16_t* containerId = reinterpret_cast<uint16_t*>(start);
    EXPECT_EQ(le16toh(*containerId), 1u);
    start += sizeof(uint16_t);
    EXPECT_EQ(*start, PLDM_ENTITY_ASSOCIAION_LOGICAL);
    start += sizeof(uint8_t);
    pldm_entity* entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 1u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 0u);
    start += sizeof(pldm_entity);
    EXPECT_EQ(*start,
              pldm_entity_get_num_children(l1, PLDM_ENTITY_ASSOCIAION_LOGICAL));
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 3u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 1u);
    start += sizeof(pldm_entity);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 3u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 2u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 1u);
    start += sizeof(pldm_entity);

    currRecHandle = nextRecHandle;

    pldm_pdr_find_record(repo, currRecHandle, &data, &size, &nextRecHandle);
    EXPECT_EQ(size, commonSize + (pldm_entity_get_num_children(
                                      l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL) *
                                  sizeof(pldm_entity)));
    start = data;
    hdr = reinterpret_cast<pldm_pdr_hdr*>(start);
    EXPECT_EQ(le32toh(hdr->record_handle), 2u);
    EXPECT_EQ(hdr->type, PLDM_PDR_ENTITY_ASSOCIATION);
    EXPECT_EQ(le16toh(hdr->length), size - sizeof(struct pldm_pdr_hdr));
    start += sizeof(pldm_pdr_hdr);
    containerId = reinterpret_cast<uint16_t*>(start);
    EXPECT_EQ(le16toh(*containerId), 1u);
    start += sizeof(uint16_t);
    EXPECT_EQ(*start, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 1u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 0u);
    start += sizeof(pldm_entity);
    EXPECT_EQ(*start, pldm_entity_get_num_children(
                          l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL));
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 2u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 1u);
    start += sizeof(pldm_entity);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 2u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 2u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 1u);
    start += sizeof(pldm_entity);

    currRecHandle = nextRecHandle;

    pldm_pdr_find_record(repo, currRecHandle, &data, &size, &nextRecHandle);
    EXPECT_EQ(size, commonSize + (pldm_entity_get_num_children(
                                      l2a, PLDM_ENTITY_ASSOCIAION_LOGICAL) *
                                  sizeof(pldm_entity)));
    start = data;
    hdr = reinterpret_cast<pldm_pdr_hdr*>(start);
    EXPECT_EQ(le32toh(hdr->record_handle), 3u);
    EXPECT_EQ(hdr->type, PLDM_PDR_ENTITY_ASSOCIATION);
    EXPECT_EQ(le16toh(hdr->length), size - sizeof(struct pldm_pdr_hdr));
    start += sizeof(pldm_pdr_hdr);
    containerId = reinterpret_cast<uint16_t*>(start);
    EXPECT_EQ(le16toh(*containerId), 2u);
    start += sizeof(uint16_t);
    EXPECT_EQ(*start, PLDM_ENTITY_ASSOCIAION_LOGICAL);
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 2u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 1u);
    start += sizeof(pldm_entity);
    EXPECT_EQ(*start, pldm_entity_get_num_children(
                          l2a, PLDM_ENTITY_ASSOCIAION_LOGICAL));
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 5);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 2u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 2u);
    start += sizeof(pldm_entity);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 5u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 3u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 2u);
    start += sizeof(pldm_entity);

    currRecHandle = nextRecHandle;

    pldm_pdr_find_record(repo, currRecHandle, &data, &size, &nextRecHandle);
    EXPECT_EQ(size, commonSize + (pldm_entity_get_num_children(
                                      l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL) *
                                  sizeof(pldm_entity)));
    start = data;
    hdr = reinterpret_cast<pldm_pdr_hdr*>(start);
    EXPECT_EQ(le32toh(hdr->record_handle), 4u);
    EXPECT_EQ(hdr->type, PLDM_PDR_ENTITY_ASSOCIATION);
    EXPECT_EQ(le16toh(hdr->length), size - sizeof(struct pldm_pdr_hdr));
    start += sizeof(pldm_pdr_hdr);
    containerId = reinterpret_cast<uint16_t*>(start);
    EXPECT_EQ(le16toh(*containerId), 2u);
    start += sizeof(uint16_t);
    EXPECT_EQ(*start, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 2u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 1u);
    start += sizeof(pldm_entity);
    EXPECT_EQ(*start, pldm_entity_get_num_children(
                          l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL));
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 4u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 2u);
    start += sizeof(pldm_entity);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 5u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 2u);
    start += sizeof(pldm_entity);

    currRecHandle = nextRecHandle;

    pldm_pdr_find_record(repo, currRecHandle, &data, &size, &nextRecHandle);
    EXPECT_EQ(size, commonSize + (pldm_entity_get_num_children(
                                      l3a, PLDM_ENTITY_ASSOCIAION_PHYSICAL) *
                                  sizeof(pldm_entity)));
    start = data;
    hdr = reinterpret_cast<pldm_pdr_hdr*>(start);
    EXPECT_EQ(le32toh(hdr->record_handle), 5u);
    EXPECT_EQ(hdr->type, PLDM_PDR_ENTITY_ASSOCIATION);
    EXPECT_EQ(le16toh(hdr->length), size - sizeof(struct pldm_pdr_hdr));
    start += sizeof(pldm_pdr_hdr);
    containerId = reinterpret_cast<uint16_t*>(start);
    EXPECT_EQ(le16toh(*containerId), 3u);
    start += sizeof(uint16_t);
    EXPECT_EQ(*start, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 4u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 2u);
    start += sizeof(pldm_entity);
    EXPECT_EQ(*start, pldm_entity_get_num_children(
                          l3a, PLDM_ENTITY_ASSOCIAION_PHYSICAL));
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 6u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 3u);
    start += sizeof(pldm_entity);

    currRecHandle = nextRecHandle;

    pldm_pdr_find_record(repo, currRecHandle, &data, &size, &nextRecHandle);
    EXPECT_EQ(size, commonSize + (pldm_entity_get_num_children(
                                      l3b, PLDM_ENTITY_ASSOCIAION_LOGICAL) *
                                  sizeof(pldm_entity)));
    start = data;
    hdr = reinterpret_cast<pldm_pdr_hdr*>(start);
    EXPECT_EQ(le32toh(hdr->record_handle), 6u);
    EXPECT_EQ(hdr->type, PLDM_PDR_ENTITY_ASSOCIATION);
    EXPECT_EQ(le16toh(hdr->length), size - sizeof(struct pldm_pdr_hdr));
    start += sizeof(pldm_pdr_hdr);
    containerId = reinterpret_cast<uint16_t*>(start);
    EXPECT_EQ(le16toh(*containerId), 4u);
    start += sizeof(uint16_t);
    EXPECT_EQ(*start, PLDM_ENTITY_ASSOCIAION_LOGICAL);
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 5u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 2u);
    start += sizeof(pldm_entity);
    EXPECT_EQ(*start, pldm_entity_get_num_children(
                          l3b, PLDM_ENTITY_ASSOCIAION_LOGICAL));
    start += sizeof(uint8_t);
    entity = reinterpret_cast<pldm_entity*>(start);
    EXPECT_EQ(le16toh(entity->entity_type), 7u);
    EXPECT_EQ(le16toh(entity->entity_instance_num), 1u);
    EXPECT_EQ(le16toh(entity->entity_container_id), 4u);

    EXPECT_EQ(nextRecHandle, 0u);

    pldm_pdr_destroy(repo);
    pldm_entity_association_tree_destroy(tree);
}

TEST(EntityAssociationPDR, testFind)
{
    //        1
    //        |
    //        2--3--4
    //        |
    //        5--6--7
    //        |  |
    //        8  9

    pldm_entity entities[9]{};

    entities[0].entity_type = 1;
    entities[1].entity_type = 2;
    entities[2].entity_type = 2;
    entities[3].entity_type = 3;
    entities[4].entity_type = 4;
    entities[5].entity_type = 5;
    entities[6].entity_type = 5;
    entities[7].entity_type = 6;
    entities[8].entity_type = 7;

    auto tree = pldm_entity_association_tree_init();

    auto l1 = pldm_entity_association_tree_add(tree, &entities[0], nullptr,
                                               PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l1, nullptr);
    auto l2a = pldm_entity_association_tree_add(
        tree, &entities[1], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2a, nullptr);
    auto l2b = pldm_entity_association_tree_add(
        tree, &entities[2], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2b, nullptr);
    auto l2c = pldm_entity_association_tree_add(
        tree, &entities[3], l1, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l2c, nullptr);
    auto l3a = pldm_entity_association_tree_add(
        tree, &entities[4], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3a, nullptr);
    auto l3b = pldm_entity_association_tree_add(
        tree, &entities[5], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3b, nullptr);
    auto l3c = pldm_entity_association_tree_add(
        tree, &entities[6], l2a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l3c, nullptr);
    auto l4a = pldm_entity_association_tree_add(
        tree, &entities[7], l3a, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l4a, nullptr);
    auto l4b = pldm_entity_association_tree_add(
        tree, &entities[8], l3b, PLDM_ENTITY_ASSOCIAION_PHYSICAL);
    EXPECT_NE(l4b, nullptr);

    pldm_entity entity{};

    entity.entity_type = 1;
    entity.entity_instance_num = 1;
    auto result = pldm_entity_association_tree_find(tree, &entity);
    EXPECT_EQ(result, l1);
    EXPECT_EQ(entity.entity_container_id, 0);

    entity.entity_type = 2;
    entity.entity_instance_num = 1;
    result = pldm_entity_association_tree_find(tree, &entity);
    EXPECT_EQ(result, l2a);
    EXPECT_EQ(entity.entity_container_id, 1);
    entity.entity_type = 2;
    entity.entity_instance_num = 2;
    result = pldm_entity_association_tree_find(tree, &entity);
    EXPECT_EQ(result, l2b);
    EXPECT_EQ(entity.entity_container_id, 1);
    entity.entity_type = 3;
    entity.entity_instance_num = 1;
    result = pldm_entity_association_tree_find(tree, &entity);
    EXPECT_EQ(result, l2c);
    EXPECT_EQ(entity.entity_container_id, 1);

    entity.entity_type = 7;
    entity.entity_instance_num = 1;
    result = pldm_entity_association_tree_find(tree, &entity);
    EXPECT_EQ(result, l4b);
    EXPECT_EQ(entity.entity_container_id, 4);

    pldm_entity_association_tree_destroy(tree);
}

TEST(EntityAssociationPDR, testExtract)
{
    std::vector<uint8_t> pdr{};
    pdr.resize(sizeof(pldm_pdr_hdr) + sizeof(pldm_pdr_entity_association) +
               sizeof(pldm_entity) * 4);
    pldm_pdr_hdr* hdr = reinterpret_cast<pldm_pdr_hdr*>(pdr.data());
    hdr->type = PLDM_PDR_ENTITY_ASSOCIATION;
    hdr->length =
        htole16(sizeof(pldm_pdr_entity_association) + sizeof(pldm_entity) * 4);

    pldm_pdr_entity_association* e =
        reinterpret_cast<pldm_pdr_entity_association*>(pdr.data() +
                                                       sizeof(pldm_pdr_hdr));
    e->container_id = htole16(1);
    e->num_children = 5;
    e->container.entity_type = htole16(1);
    e->container.entity_instance_num = htole16(1);
    e->container.entity_container_id = htole16(0);

    pldm_entity* entity = e->children;
    entity->entity_type = htole16(2);
    entity->entity_instance_num = htole16(1);
    entity->entity_container_id = htole16(1);
    ++entity;
    entity->entity_type = htole16(3);
    entity->entity_instance_num = htole16(1);
    entity->entity_container_id = htole16(1);
    ++entity;
    entity->entity_type = htole16(4);
    entity->entity_instance_num = htole16(1);
    entity->entity_container_id = htole16(1);
    ++entity;
    entity->entity_type = htole16(5);
    entity->entity_instance_num = htole16(1);
    entity->entity_container_id = htole16(1);
    ++entity;
    entity->entity_type = htole16(6);
    entity->entity_instance_num = htole16(1);
    entity->entity_container_id = htole16(1);

    size_t num{};
    pldm_entity* out = nullptr;
    pldm_entity_association_pdr_extract(pdr.data(), pdr.size(), &num, &out);
    EXPECT_EQ(num, (unsigned)e->num_children + 1);
    EXPECT_EQ(out[0].entity_type, 1u);
    EXPECT_EQ(out[0].entity_instance_num, 1u);
    EXPECT_EQ(out[0].entity_container_id, 0u);
    EXPECT_EQ(out[1].entity_type, 2u);
    EXPECT_EQ(out[1].entity_instance_num, 1u);
    EXPECT_EQ(out[1].entity_container_id, 1u);
    EXPECT_EQ(out[2].entity_type, 3u);
    EXPECT_EQ(out[2].entity_instance_num, 1u);
    EXPECT_EQ(out[2].entity_container_id, 1u);
    EXPECT_EQ(out[3].entity_type, 4u);
    EXPECT_EQ(out[3].entity_instance_num, 1u);
    EXPECT_EQ(out[3].entity_container_id, 1u);
    EXPECT_EQ(out[4].entity_type, 5u);
    EXPECT_EQ(out[4].entity_instance_num, 1u);
    EXPECT_EQ(out[4].entity_container_id, 1u);
    EXPECT_EQ(out[5].entity_type, 6u);
    EXPECT_EQ(out[5].entity_instance_num, 1u);
    EXPECT_EQ(out[5].entity_container_id, 1u);

    free(out);
}
