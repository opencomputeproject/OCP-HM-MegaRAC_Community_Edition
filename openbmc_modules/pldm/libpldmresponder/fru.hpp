#pragma once

#include "config.h"

#include "libpldm/fru.h"
#include "libpldm/pdr.h"

#include "fru_parser.hpp"
#include "pldmd/handler.hpp"

#include <sdbusplus/message.hpp>

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace pldm
{

namespace responder
{

namespace dbus
{

using Value =
    std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, double, std::string, std::vector<uint8_t>>;
using PropertyMap = std::map<Property, Value>;
using InterfaceMap = std::map<Interface, PropertyMap>;
using ObjectValueTree = std::map<sdbusplus::message::object_path, InterfaceMap>;
using ObjectPath = std::string;

} // namespace dbus

/** @class FruImpl
 *
 *  @brief Builds the PLDM FRU table containing the FRU records
 */
class FruImpl
{
  public:
    /* @brief Header size for FRU record, it includes the FRU record set
     *        identifier, FRU record type, Number of FRU fields, Encoding type
     *        of FRU fields
     */
    static constexpr size_t recHeaderSize =
        sizeof(struct pldm_fru_record_data_format) -
        sizeof(struct pldm_fru_record_tlv);

    /** @brief Constructor for FruImpl, the configPath is consumed to build the
     *         FruParser object.
     *
     *  @param[in] configPath - path to the directory containing config files
     *                          for PLDM FRU
     *  @param[in] pdrRepo - opaque pointer to PDR repository
     *  @param[in] entityTree - opaque pointer to the entity association tree
     */
    FruImpl(const std::string& configPath, pldm_pdr* pdrRepo,
            pldm_entity_association_tree* entityTree) :
        parser(configPath),
        pdrRepo(pdrRepo), entityTree(entityTree)
    {}

    /** @brief Total length of the FRU table in bytes, this excludes the pad
     *         bytes and the checksum.
     *
     *  @return size of the FRU table
     */
    uint32_t size() const
    {
        return table.size() - padBytes;
    }

    /** @brief The checksum of the contents of the FRU table
     *
     *  @return checksum
     */
    uint32_t checkSum() const
    {
        return checksum;
    }

    /** @brief Number of record set identifiers in the FRU tables
     *
     *  @return number of record set identifiers
     */
    uint16_t numRSI() const
    {
        return rsi;
    }

    /** @brief The number of FRU records in the table
     *
     *  @return number of FRU records
     */
    uint16_t numRecords() const
    {
        return numRecs;
    }

    /** @brief Get the FRU table
     *
     *  @param[out] - Populate response with the FRU table
     */
    void getFRUTable(Response& response);

    /** @brief Get FRU Record Table By Option
     *  @param[out] response - Populate response with the FRU table got by
     *                         options
     *  @param[in] fruTableHandle - The fru table handle
     *  @param[in] recordSetIdentifer - The record set identifier
     *  @param[in] recordType - The record type
     *  @param[in] fieldType - The field type
     */
    int getFRURecordByOption(Response& response, uint16_t fruTableHandle,
                             uint16_t recordSetIdentifer, uint8_t recordType,
                             uint8_t fieldType);

    /** @brief FRU table is built by processing the D-Bus inventory namespace
     *         based on the config files for FRU. The table is populated based
     *         on the isBuilt flag.
     */
    void buildFRUTable();

  private:
    uint16_t nextRSI()
    {
        return ++rsi;
    }

    uint16_t rsi = 0;
    uint16_t numRecs = 0;
    uint8_t padBytes = 0;
    std::vector<uint8_t> table;
    uint32_t checksum = 0;
    bool isBuilt = false;

    fru_parser::FruParser parser;
    pldm_pdr* pdrRepo;
    pldm_entity_association_tree* entityTree;

    std::map<dbus::ObjectPath, pldm_entity_node*> objToEntityNode{};

    /** @brief populateRecord builds the FRU records for an instance of FRU and
     *         updates the FRU table with the FRU records.
     *
     *  @param[in] interfaces - D-Bus interfaces and the associated property
     *                          values for the FRU
     *  @param[in] recordInfos - FRU record info to build the FRU records
     *  @param[in/out] entity - PLDM entity corresponding to FRU instance
     */
    void populateRecords(const pldm::responder::dbus::InterfaceMap& interfaces,
                         const fru_parser::FruRecordInfos& recordInfos,
                         const pldm_entity& entity);
};

namespace fru
{

class Handler : public CmdHandler
{

  public:
    Handler(const std::string& configPath, pldm_pdr* pdrRepo,
            pldm_entity_association_tree* entityTree) :
        impl(configPath, pdrRepo, entityTree)
    {
        handlers.emplace(PLDM_GET_FRU_RECORD_TABLE_METADATA,
                         [this](const pldm_msg* request, size_t payloadLength) {
                             return this->getFRURecordTableMetadata(
                                 request, payloadLength);
                         });

        handlers.emplace(PLDM_GET_FRU_RECORD_TABLE,
                         [this](const pldm_msg* request, size_t payloadLength) {
                             return this->getFRURecordTable(request,
                                                            payloadLength);
                         });
        handlers.emplace(PLDM_GET_FRU_RECORD_BY_OPTION,
                         [this](const pldm_msg* request, size_t payloadLength) {
                             return this->getFRURecordByOption(request,
                                                               payloadLength);
                         });
    }

    /** @brief Handler for Get FRURecordTableMetadata
     *
     *  @param[in] request - Request message payload
     *  @param[in] payloadLength - Request payload length
     *
     *  @return PLDM response message
     */
    Response getFRURecordTableMetadata(const pldm_msg* request,
                                       size_t payloadLength);

    /** @brief Handler for GetFRURecordTable
     *
     *  @param[in] request - Request message payload
     *  @param[in] payloadLength - Request payload length
     *
     *  @return PLDM response message
     */
    Response getFRURecordTable(const pldm_msg* request, size_t payloadLength);

    /** @brief Build FRU table is bnot already built
     *
     */
    void buildFRUTable()
    {
        impl.buildFRUTable();
    }

    /** @brief Handler for GetFRURecordByOption
     *
     *  @param[in] request - Request message payload
     *  @param[in] payloadLength - Request payload length
     *
     *  @return PLDM response message
     */
    Response getFRURecordByOption(const pldm_msg* request,
                                  size_t payloadLength);

  private:
    FruImpl impl;
};

} // namespace fru

} // namespace responder

} // namespace pldm
