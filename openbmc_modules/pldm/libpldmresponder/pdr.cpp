#include "pdr.hpp"

#include "pdr_state_effecter.hpp"

namespace pldm
{

namespace responder
{

namespace pdr
{
using namespace pldm::responder::pdr_utils;

void getRepoByType(const Repo& inRepo, Repo& outRepo, Type pdrType)
{
    uint8_t* pdrData = nullptr;
    uint32_t pdrSize{};
    auto record = pldm_pdr_find_record_by_type(inRepo.getPdr(), pdrType, NULL,
                                               &pdrData, &pdrSize);
    while (record)
    {
        PdrEntry pdrEntry{};
        pdrEntry.data = pdrData;
        pdrEntry.size = pdrSize;
        pdrEntry.handle.recordHandle = inRepo.getRecordHandle(record);
        outRepo.addRecord(pdrEntry);

        pdrData = nullptr;
        pdrSize = 0;
        record = pldm_pdr_find_record_by_type(inRepo.getPdr(), pdrType, record,
                                              &pdrData, &pdrSize);
    }
}

const pldm_pdr_record* getRecordByHandle(const RepoInterface& pdrRepo,
                                         RecordHandle recordHandle,
                                         PdrEntry& pdrEntry)
{
    uint8_t* pdrData = nullptr;
    auto record =
        pldm_pdr_find_record(pdrRepo.getPdr(), recordHandle, &pdrData,
                             &pdrEntry.size, &pdrEntry.handle.nextRecordHandle);
    if (record)
    {
        pdrEntry.data = pdrData;
    }

    return record;
}

} // namespace pdr
} // namespace responder
} // namespace pldm
