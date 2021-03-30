#include "internal_sys_mock.hpp"

#include <ipmiblob/ipmi_errors.hpp>
#include <ipmiblob/ipmi_handler.hpp>

namespace ipmiblob
{

using ::testing::_;
using ::testing::Return;

TEST(IpmiHandlerTest, OpenAllFails)
{
    /* Open against all device files fail. */
    internal::InternalSysMock sysMock;
    IpmiHandler ipmi(&sysMock);

    EXPECT_CALL(sysMock, open(_, _)).WillRepeatedly(Return(-1));
    EXPECT_THROW(ipmi.open(), IpmiException);
}

} // namespace ipmiblob
