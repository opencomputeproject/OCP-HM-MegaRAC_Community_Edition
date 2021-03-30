// A basic unit test that runs on a BMC (qemu or hardware)

#include <getopt.h>
#include <systemd/sd-journal.h>

#include <cstring>
#include <iostream>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <sstream>

using namespace phosphor;
using namespace logging;

const char* usage = "Usage: logging-test [OPTION]          \n\n\
Options:                                                     \n\
[NONE]                          Default test case.           \n\
-h, --help                      Display this usage text.     \n\
-c, --commit <string>           Commit desired error.      \n\n\
Valid errors to commit:                                      \n\
AutoTestSimple, AutoTestCreateAndCommit\n";

// validate the journal metadata equals the input value
int validate_journal(const char* i_entry, const char* i_value)
{
    sd_journal* journal;
    const void* data;
    size_t l;
    int rc;
    bool validated = false;

    rc = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (rc < 0)
    {
        std::cerr << "Failed to open journal: " << strerror(-rc) << "\n";
        return 1;
    }
    rc = sd_journal_query_unique(journal, i_entry);
    if (rc < 0)
    {
        std::cerr << "Failed to query journal: " << strerror(-rc) << "\n";
        return 1;
    }
    SD_JOURNAL_FOREACH_UNIQUE(journal, data, l)
    {
        std::string journ_entry((const char*)data);
        std::cout << journ_entry << "\n";
        if (journ_entry.find(i_value) != std::string::npos)
        {
            std::cout << "We found it!\n";
            validated = true;
            break;
        }
    }

    sd_journal_close(journal);

    rc = (validated) ? 0 : 1;
    if (rc)
    {
        std::cerr << "Failed to find " << i_entry << " with value " << i_value
                  << " in journal!"
                  << "\n";
    }

    return rc;
}

int elog_test()
{
    // TEST 1 - Basic log
    log<level::DEBUG>("Basic phosphor logging test");

    // TEST 2 - Log with metadata field
    const char* file_name = "phosphor_logging_test.txt";
    int number = 0xFEFE;
    log<level::DEBUG>(
        "phosphor logging test with attribute",
        entry("FILE_NAME_WITH_NUM_TEST=%s_%x", file_name, number));

    // Now read back and verify our data made it into the journal
    int rc = validate_journal("FILE_NAME_WITH_NUM_TEST",
                              "phosphor_logging_test.txt_fefe");
    if (rc)
        return (rc);

    // TEST 3 - Create error log with 2 meta data fields (rvalue and lvalue)
    number = 0x1234;
    const char* test_string = "/tmp/test_string/";
    try
    {
        elog<example::xyz::openbmc_project::Example::Elog::TestErrorOne>(
            example::xyz::openbmc_project::Example::Elog::TestErrorOne::ERRNUM(
                number),
            example::xyz::openbmc_project::Example::Elog::TestErrorOne::
                FILE_PATH(test_string),
            example::xyz::openbmc_project::Example::Elog::TestErrorOne::
                FILE_NAME("elog_test_3.txt"),
            example::xyz::openbmc_project::Example::Elog::TestErrorTwo::
                DEV_ADDR(0xDEADDEAD),
            example::xyz::openbmc_project::Example::Elog::TestErrorTwo::DEV_ID(
                100),
            example::xyz::openbmc_project::Example::Elog::TestErrorTwo::
                DEV_NAME("test case 3"));
    }
    catch (example::xyz::openbmc_project::Example::Elog::TestErrorOne& e)
    {
        std::cout << "elog exception caught: " << e.what() << std::endl;
    }

    // Reduce our error namespaces
    using namespace example::xyz::openbmc_project::Example::Elog;

    // Now read back and verify our data made it into the journal
    std::stringstream stream;
    stream << std::hex << number;
    rc = validate_journal(TestErrorOne::ERRNUM::str_short,
                          std::string(stream.str()).c_str());
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorOne::FILE_PATH::str_short, test_string);
    if (rc)
        return (rc);

    rc =
        validate_journal(TestErrorOne::FILE_NAME::str_short, "elog_test_3.txt");
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorTwo::DEV_ADDR::str_short, "0xDEADDEAD");
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorTwo::DEV_ID::str_short, "100");
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorTwo::DEV_NAME::str_short, "test case 3");
    if (rc)
        return (rc);

    // TEST 4 - Create error log with previous entry use
    number = 0x9876;
    try
    {
        elog<TestErrorOne>(
            TestErrorOne::ERRNUM(number), prev_entry<TestErrorOne::FILE_PATH>(),
            TestErrorOne::FILE_NAME("elog_test_4.txt"),
            TestErrorTwo::DEV_ADDR(0xDEADDEAD), TestErrorTwo::DEV_ID(100),
            TestErrorTwo::DEV_NAME("test case 4"));
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cout << "elog exception caught: " << e.what() << std::endl;
    }

    // Now read back and verify our data made it into the journal
    stream.str("");
    stream << std::hex << number;
    rc = validate_journal(TestErrorOne::ERRNUM::str_short,
                          std::string(stream.str()).c_str());
    if (rc)
        return (rc);

    // This should just be equal to what we put in test 3
    rc = validate_journal(TestErrorOne::FILE_PATH::str_short, test_string);
    if (rc)
        return (rc);

    rc =
        validate_journal(TestErrorOne::FILE_NAME::str_short, "elog_test_4.txt");
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorTwo::DEV_ADDR::str_short, "0xDEADDEAD");
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorTwo::DEV_ID::str_short, "100");
    if (rc)
        return (rc);

    rc = validate_journal(TestErrorTwo::DEV_NAME::str_short, "test case 4");
    if (rc)
        return (rc);

    // Compile fail tests

    // Simple test to prove we fail to compile due to missing param
    // elog<TestErrorOne>(TestErrorOne::ERRNUM(1),
    //                   TestErrorOne::FILE_PATH("test"));

    // Simple test to prove we fail to compile due to invalid param
    // elog<TestErrorOne>(TestErrorOne::ERRNUM(1),
    //                   TestErrorOne::FILE_PATH("test"),
    //                   TestErrorOne::FILE_NAME(1));

    return 0;
}

void commitError(const char* text)
{
    if (std::strcmp(text, "AutoTestSimple") == 0)
    {
        try
        {
            elog<example::xyz::openbmc_project::Example::Elog::AutoTestSimple>(
                example::xyz::openbmc_project::Example::Elog::AutoTestSimple::
                    STRING("FOO"));
        }
        catch (example::xyz::openbmc_project::Example::Elog::AutoTestSimple& e)
        {
            std::cout << "elog exception caught: " << e.what() << std::endl;
            commit(e.name());
        }
    }
    else if (std::strcmp(text, "AutoTestCreateAndCommit") == 0)
    {
        report<example::xyz::openbmc_project::Example::Elog::AutoTestSimple>(
            example::xyz::openbmc_project::Example::Elog::AutoTestSimple::
                STRING("FOO"));
    }

    return;
}

int main(int argc, char* argv[])
{
    char arg;

    if (argc == 1)
        return elog_test();

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"commit", required_argument, 0, 'c'},
        {0, 0, 0, 0}};
    int option_index = 0;

    while ((arg = getopt_long(argc, argv, "hc:", long_options,
                              &option_index)) != -1)
    {
        switch (arg)
        {
            case 'c':
                commitError(optarg);
                return 0;
            case 'h':
            case '?':
                std::cerr << usage;
                return 1;
        }
    }

    return 0;
}
