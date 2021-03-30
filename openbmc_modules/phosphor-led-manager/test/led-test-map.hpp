#include "ledlayout.hpp"

#include <map>
#include <set>
#include <string>

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    singleLedOn = {
        {"/xyz/openbmc_project/ledmanager/groups/SingleLed",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    singleLedBlink = {
        {"/xyz/openbmc_project/ledmanager/groups/SingleLed",
         {
             {"One", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    singleLedBlinkOverrideOn = {
        {"/xyz/openbmc_project/ledmanager/groups/SingleLed",
         {
             {"One", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    multipleLedsOn = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLeds",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    multipleLedsBlink = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLeds",
         {
             {"One", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
             {"Two", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
             {"Three", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    multipleLedsOnAndBlink = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsMix",
         {
             {"One", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
             {"Two", phosphor::led::Layout::On, 0,
              phosphor::led::Layout::Blink},
             {"Three", phosphor::led::Layout::Blink, 0,
              phosphor::led::Layout::On},
             {"Four", phosphor::led::Layout::On, 0,
              phosphor::led::Layout::Blink},
             {"Five", phosphor::led::Layout::On, 0,
              phosphor::led::Layout::Blink},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithDistinctLEDsOn = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Four", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
             {"Five", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithOneComonLEDOn = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Four", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithOneComonLEDOnOneLEDBlinkPriority = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Four", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithOneComonLEDOnPriority = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::On},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Four", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithMultiplComonLEDOn = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Seven", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithMultipleComonLEDInDifferentState = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::On},
             {"Four", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::On},
             {"Five", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
         }},
};

static const std::map<std::string, std::set<phosphor::led::Layout::LedAction>>
    twoGroupsWithMultipleComonLEDInDifferentStateDiffPriority = {
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsASet",
         {
             {"One", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Two", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
             {"Four", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Ten", phosphor::led::Layout::Blink, 0, 0,
              phosphor::led::Layout::Blink},
         }},
        {"/xyz/openbmc_project/ledmanager/groups/MultipleLedsBSet",
         {
             {"Two", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Three", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
             {"Five", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Six", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::On},
             {"Ten", phosphor::led::Layout::On, 0, 0,
              phosphor::led::Layout::Blink},
         }},
};
