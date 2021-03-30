#include "libpldm/base.h"

#include "pldmd/invoker.hpp"

#include <stdexcept>

#include <gtest/gtest.h>

using namespace pldm;
using namespace pldm::responder;
constexpr Command testCmd = 0xFF;
constexpr Type testType = 0xFF;

class TestHandler : public CmdHandler
{
  public:
    TestHandler()
    {
        handlers.emplace(testCmd,
                         [this](const pldm_msg* request, size_t payloadLength) {
                             return this->handle(request, payloadLength);
                         });
    }

    Response handle(const pldm_msg* /*request*/, size_t /*payloadLength*/)
    {
        return {100, 200};
    }
};

TEST(CcOnlyResponse, testEncode)
{
    std::vector<uint8_t> requestMsg(sizeof(pldm_msg_hdr));
    auto request = reinterpret_cast<pldm_msg*>(requestMsg.data());
    encode_get_types_req(0, request);

    auto responseMsg = CmdHandler::ccOnlyResponse(request, PLDM_ERROR);
    std::vector<uint8_t> expectMsg = {0, 0, 4, 1};
    EXPECT_EQ(responseMsg, expectMsg);
}

TEST(Registration, testSuccess)
{
    Invoker invoker{};
    invoker.registerHandler(testType, std::make_unique<TestHandler>());
    auto result = invoker.handle(testType, testCmd, nullptr, 0);
    ASSERT_EQ(result[0], 100);
    ASSERT_EQ(result[1], 200);
}

TEST(Registration, testFailure)
{
    Invoker invoker{};
    ASSERT_THROW(invoker.handle(testType, testCmd, nullptr, 0),
                 std::out_of_range);
    invoker.registerHandler(testType, std::make_unique<TestHandler>());
    uint8_t badCmd = 0xFE;
    ASSERT_THROW(invoker.handle(testType, badCmd, nullptr, 0),
                 std::out_of_range);
}
