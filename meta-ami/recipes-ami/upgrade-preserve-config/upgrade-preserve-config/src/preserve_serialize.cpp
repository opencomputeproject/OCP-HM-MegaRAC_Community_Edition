/**
 * boilerplate code taken from phosphor-snmp
 */

#include "config.h"
#include "preserve_serialize.hpp"
#include "preserve.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

#include <fstream>

CEREAL_CLASS_VERSION(phosphor::software::preserve::PreserveConf,
    CLASS_VERSION);

namespace phosphor
{
namespace software
{
namespace preserve
{

/** @brief Function required by Cereal to perform serialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] archive - reference to Cereal archive.
 *  @param[in] manager - const reference to preservation manager info.
 *  @param[in] version - Class version that enables handling
 *                       a serialized data across code levels
 */
template <class Archive>
void save(Archive& archive, const PreserveConf& manager, const std::uint32_t version)
{
    archive(manager.iPMI(), manager.user(), manager.lDAP(), manager.certificates(),
            manager.hostname(), manager.sEL(), manager.sDR(), manager.network());
}

/** @brief Function required by Cereal to perform deserialization.
 *  @tparam Archive - Cereal archive type (binary in our case).
 *  @param[in] archive - reference to Cereal archive.
 *  @param[in] manager - reference to preservation manager info.
 *  @param[in] version - Class version that enables handling
 *                       a serialized data across code levels
 */
template <class Archive>
void load(Archive& archive, PreserveConf& manager, const std::uint32_t version)
{
    bool ipmi{}, user{}, ldap{}, certificates{}, hostname{}, sel{}, sdr{}, network{};

    archive(ipmi, user, ldap, certificates, hostname, sel, sdr, network);

    manager.iPMI(ipmi);
    manager.user(user);
    manager.lDAP(ldap);
    manager.certificates(certificates);
    manager.hostname(hostname);
    manager.sEL(sel);
    manager.sDR(sdr);
    manager.network(network);
}

fs::path serialize(const PreserveConf& manager, const fs::path& dir)
{
    fs::path fileName = dir;
    fs::create_directories(dir);
    fileName /= UPGRADE_CONF_PERSIST_FILE;

    std::ofstream os(fileName.string(), std::ios::binary);
    cereal::BinaryOutputArchive oarchive(os);
    oarchive(manager);
    return fileName;
}

bool deserialize(const fs::path& path, PreserveConf& manager)
{
    try
    {
        if (fs::exists(path))
        {
            std::ifstream is(path.c_str(), std::ios::in | std::ios::binary);
            cereal::BinaryInputArchive iarchive(is);
            iarchive(manager);
            return true;
        }
        return false;
    }
    catch (cereal::Exception& e)
    {
        std::error_code ec;
        fs::remove(path, ec);
        return false;
    }
    catch (const fs::filesystem_error& e)
    {
        return false;
    }
}

} // namespace preserve
} // namespace software
} // namespace phosphor
