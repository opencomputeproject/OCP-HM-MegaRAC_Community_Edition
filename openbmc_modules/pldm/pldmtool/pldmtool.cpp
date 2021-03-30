#include "pldm_base_cmd.hpp"
#include "pldm_bios_cmd.hpp"
#include "pldm_cmd_helper.hpp"
#include "pldm_fru_cmd.hpp"
#include "pldm_platform_cmd.hpp"
#include "pldmtool/oem/ibm/pldm_oem_ibm.hpp"

#include <CLI/CLI.hpp>

namespace pldmtool
{

namespace raw
{

using namespace pldmtool::helper;

namespace
{
std::vector<std::unique_ptr<CommandInterface>> commands;
}

class RawOp : public CommandInterface
{
  public:
    ~RawOp() = default;
    RawOp() = delete;
    RawOp(const RawOp&) = delete;
    RawOp(RawOp&&) = default;
    RawOp& operator=(const RawOp&) = delete;
    RawOp& operator=(RawOp&&) = default;

    explicit RawOp(const char* type, const char* name, CLI::App* app) :
        CommandInterface(type, name, app)
    {
        app->add_option("-d,--data", rawData, "raw data")
            ->required()
            ->expected(-3);
    }
    std::pair<int, std::vector<uint8_t>> createRequestMsg() override

    {
        return {PLDM_SUCCESS, rawData};
    }

    void parseResponseMsg(pldm_msg* /* responsePtr */,
                          size_t /* payloadLength */) override
    {}

  private:
    std::vector<uint8_t> rawData;
};

void registerCommand(CLI::App& app)
{
    auto raw =
        app.add_subcommand("raw", "send a raw request and print response");
    commands.push_back(std::make_unique<RawOp>("raw", "raw", raw));
}

} // namespace raw
} // namespace pldmtool

int main(int argc, char** argv)
{

    CLI::App app{"PLDM requester tool for OpenBMC"};
    app.require_subcommand(1)->ignore_case();

    pldmtool::raw::registerCommand(app);
    pldmtool::base::registerCommand(app);
    pldmtool::bios::registerCommand(app);
    pldmtool::platform::registerCommand(app);
    pldmtool::fru::registerCommand(app);

#ifdef OEM_IBM
    pldmtool::oem_ibm::registerCommand(app);
#endif

    CLI11_PARSE(app, argc, argv);
    return 0;
}
