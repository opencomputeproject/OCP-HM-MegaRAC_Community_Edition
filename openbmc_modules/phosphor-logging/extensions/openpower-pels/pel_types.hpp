#pragma once

namespace openpower
{
namespace pels
{

/**
 * @brief Useful component IDs
 */
enum class ComponentID
{
    phosphorLogging = 0x2000
};

/**
 * @brief PEL section IDs
 */
enum class SectionID
{
    privateHeader = 0x5048,      // 'PH'
    userHeader = 0x5548,         // 'UH'
    primarySRC = 0x5053,         // 'PS'
    secondarySRC = 0x5353,       // 'SS'
    extendedUserHeader = 0x4548, // 'EH'
    failingMTMS = 0x4D54,        // 'MT'
    dumpLocation = 0x4448,       // 'DH'
    firmwareError = 0x5357,      // 'SW'
    impactedPart = 0x4C50,       // 'LP'
    logicalResource = 0x4C52,    // 'LR'
    hmcID = 0x484D,              // 'HM'
    epow = 0x4550,               // 'EP'
    ioEvent = 0x4945,            // 'IE'
    mfgInfo = 0x4D49,            // 'MI'
    callhome = 0x4348,           // 'CH'
    userData = 0x5544,           // 'UD'
    envInfo = 0x4549,            // 'EI'
    extUserData = 0x4544         // 'ED'
};

/**
 * @brief Useful SRC types
 */
enum class SRCType
{
    bmcError = 0xBD,
    powerError = 0x11
};

/**
 * @brief Creator IDs
 */
enum class CreatorID
{
    fsp = 'E',
    hmc = 'C',
    hostboot = 'B',
    ioDrawer = 'M',
    occ = 'T',
    openBMC = 'O',
    partFW = 'L',
    phyp = 'H',
    powerControl = 'W',
    powerNV = 'P',
    sapphire = 'K',
    slic = 'S',
};

/**
 * @brief Useful event scope values
 */
enum class EventScope
{
    entirePlatform = 0x03
};

/**
 * @brief Useful event type values
 */
enum class EventType
{
    notApplicable = 0x00,
    miscInformational = 0x01,
    tracing = 0x02
};

/**
 * @brief The major types of severity values, based on the
 *        the left nibble of the severity value.
 */
enum class SeverityType
{
    nonError = 0x00,
    recovered = 0x10,
    predictive = 0x20,
    unrecoverable = 0x40,
    critical = 0x50,
    diagnostic = 0x60,
    symptom = 0x70
};

/**
 * @brief The Action Flags values with the bit
 *        numbering needed by std::bitset.
 *
 * Not an enum class so that casting isn't needed
 * by the bitset operations.
 */
enum ActionFlagsBits
{
    serviceActionFlagBit = 15,       // 0x8000
    hiddenFlagBit = 14,              // 0x4000
    reportFlagBit = 13,              // 0x2000
    dontReportToHostFlagBit = 12,    // 0x1000
    callHomeFlagBit = 11,            // 0x0800
    isolationIncompleteFlagBit = 10, // 0x0400
    spCallHomeFlagBit = 8,           // 0x0100
    osSWErrorBit = 7,                // 0x0080
    osHWErrorBit = 6                 // 0x0040
};

/**
 * @brief The PEL transmission states
 */
enum class TransmissionState
{
    newPEL = 0,
    badPEL = 1,
    sent = 2,
    acked = 3
};

/**
 * @brief Callout priority values
 */
enum class CalloutPriority
{
    high = 'H',
    medium = 'M',
    mediumGroupA = 'A',
    mediumGroupB = 'B',
    mediumGroupC = 'C',
    low = 'L'
};

} // namespace pels
} // namespace openpower
