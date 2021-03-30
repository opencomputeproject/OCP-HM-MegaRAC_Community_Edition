#include "user_mgr.hpp"
#include <gmock/gmock.h>

namespace phosphor
{
namespace user
{

constexpr auto objpath = "/dummy/user";

class MockManager : public UserMgr
{
  public:
    MockManager(sdbusplus::bus::bus& bus, const char* path) :
        UserMgr(bus, objpath)
    {
    }

    MOCK_METHOD1(getLdapGroupName, std::string(const std::string& userName));
    MOCK_METHOD0(getPrivilegeMapperObject, DbusUserObj());
    MOCK_METHOD1(userLockedForFailedAttempt, bool(const std::string& userName));
    MOCK_METHOD1(userPasswordExpired, bool(const std::string& userName));

    friend class TestUserMgr;
};

} // namespace user
} // namespace phosphor
