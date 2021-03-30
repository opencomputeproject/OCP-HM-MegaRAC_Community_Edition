#include "elog_meta.hpp"

#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/exception.hpp>

using namespace phosphor::logging;

int main(int argc, char** argv)
{
    if (2 != argc)
    {
        std::cerr << "usage: callout-test <sysfs path>" << std::endl;
        return -1;
    }

    using namespace example::xyz::openbmc_project::Example::Elog;
    try
    {
        elog<TestCallout>(TestCallout::DEV_ADDR(0xDEADEAD),
                          TestCallout::CALLOUT_ERRNO_TEST(0),
                          TestCallout::CALLOUT_DEVICE_PATH_TEST(argv[1]));
    }
    catch (TestCallout& e)
    {
        commit(e.name());
    }

    return 0;
}
