#include "snmp_notification.hpp"
#include <gtest/gtest.h>
#include <netinet/in.h>

namespace phosphor
{
namespace network
{
namespace snmp
{

constexpr auto ERROR_NOTIF_FIELD_COUNT = 4;

class TestErrorNotification : public testing::Test
{
  public:
    OBMCErrorNotification notif;
    TestErrorNotification()
    {
        // Empty
    }
    std::vector<Object> getFieldOIDList()
    {
        return notif.getFieldOIDList();
    }
};

TEST_F(TestErrorNotification, VerifyErrorNotificationFields)
{
    auto oidList = getFieldOIDList();

    // verify the number of the fields in the notification.
    EXPECT_EQ(ERROR_NOTIF_FIELD_COUNT, oidList.size());

    // Verify the type of each field.
    EXPECT_EQ(ASN_UNSIGNED, std::get<Type>(oidList[0]));

    EXPECT_EQ(ASN_OPAQUE_U64, std::get<Type>(oidList[1]));
    EXPECT_EQ(ASN_INTEGER, std::get<Type>(oidList[2]));
    EXPECT_EQ(ASN_OCTET_STR, std::get<Type>(oidList[3]));
}

TEST_F(TestErrorNotification, GetASNType)
{
    auto type = getASNType<uint32_t>();
    EXPECT_EQ(ASN_UNSIGNED, type);

    type = getASNType<uint64_t>();
    EXPECT_EQ(ASN_OPAQUE_U64, type);

    type = getASNType<int32_t>();
    EXPECT_EQ(ASN_INTEGER, type);

    type = getASNType<std::string>();
    EXPECT_EQ(ASN_OCTET_STR, type);
}

} // namespace snmp
} // namespace network
} // namespace phosphor
