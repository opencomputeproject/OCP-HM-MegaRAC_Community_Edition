#include "extensions/openpower-pels/pel_rules.hpp"

#include <gtest/gtest.h>

using namespace openpower::pels;

struct CheckParams
{
    // pel_rules::check() inputs
    uint16_t actionFlags;
    uint8_t eventType;
    uint8_t severity;

    // pel_rules::check() expected outputs
    uint16_t expectedActionFlags;
    uint8_t expectedEventType;
};

const uint8_t sevInfo = 0x00;
const uint8_t sevRecovered = 0x10;
const uint8_t sevPredictive = 0x20;
const uint8_t sevUnrecov = 0x40;
const uint8_t sevCrit = 0x50;
const uint8_t sevDiagnostic = 0x60;
const uint8_t sevSymptom = 0x70;

const uint8_t typeNA = 0x00;
const uint8_t typeMisc = 0x01;
const uint8_t typeTracing = 0x02;
const uint8_t typeDumpNotif = 0x08;

TEST(PELRulesTest, TestCheckRules)
{
    // Impossible to cover all combinations, but
    // do some interesting ones.
    std::vector<CheckParams> testParams{
        // Informational errors w/ empty action flags
        // and different event types.
        {0, typeNA, sevInfo, 0x6000, typeMisc},
        {0, typeMisc, sevInfo, 0x6000, typeMisc},
        {0, typeTracing, sevInfo, 0x6000, typeTracing},
        {0, typeDumpNotif, sevInfo, 0x2000, typeDumpNotif},

        // Informational errors with wrong action flags
        {0x8900, typeNA, sevInfo, 0x6000, typeMisc},

        // Informational errors with extra valid action flags
        {0x00C0, typeMisc, sevInfo, 0x60C0, typeMisc},

        // Informational - don't report
        {0x1000, typeMisc, sevInfo, 0x5000, typeMisc},

        // Recovered will report as hidden
        {0, typeNA, sevRecovered, 0x6000, typeNA},

        // The 5 error severities will have:
        // service action, report, call home
        {0, typeNA, sevPredictive, 0xA800, typeNA},
        {0, typeNA, sevUnrecov, 0xA800, typeNA},
        {0, typeNA, sevCrit, 0xA800, typeNA},
        {0, typeNA, sevDiagnostic, 0xA800, typeNA},
        {0, typeNA, sevSymptom, 0xA800, typeNA}};

    for (const auto& entry : testParams)
    {
        auto [actionFlags, type] = pel_rules::check(
            entry.actionFlags, entry.eventType, entry.severity);

        EXPECT_EQ(actionFlags, entry.expectedActionFlags);
        EXPECT_EQ(type, entry.expectedEventType);
    }
}
