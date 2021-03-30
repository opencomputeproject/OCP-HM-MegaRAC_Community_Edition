#include "Utils.hpp"

#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

#include <variant>

#include "gtest/gtest.h"

TEST(TemplateCharReplace, replaceOneInt)
{
    nlohmann::json j = {{"foo", "$bus"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["BUS"] = 23;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = 23;
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, replaceOneStr)
{
    nlohmann::json j = {{"foo", "$TEST"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = std::string("Test");

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "Test";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, replaceSecondStr)
{
    nlohmann::json j = {{"foo", "the $TEST"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = std::string("Test");

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "the Test";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, replaceMiddleStr)
{
    nlohmann::json j = {{"foo", "the $TEST worked"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = std::string("Test");

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "the Test worked";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, replaceLastStr)
{
    nlohmann::json j = {{"foo", "the Test $TEST"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 23;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "the Test 23";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, increment)
{
    nlohmann::json j = {{"foo", "3 plus 1 equals $TEST + 1"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 3;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "3 plus 1 equals 4";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, decrement)
{
    nlohmann::json j = {{"foo", "3 minus 1 equals $TEST - 1 !"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 3;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "3 minus 1 equals 2 !";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, modulus)
{
    nlohmann::json j = {{"foo", "3 mod 2 equals $TEST % 2"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 3;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "3 mod 2 equals 1";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, multiply)
{
    nlohmann::json j = {{"foo", "3 * 2 equals $TEST * 2"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 3;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "3 * 2 equals 6";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, divide)
{
    nlohmann::json j = {{"foo", "4 / 2 equals $TEST / 2"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 4;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "4 / 2 equals 2";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, multiMath)
{
    nlohmann::json j = {{"foo", "4 * 2 % 6 equals $TEST * 2 % 6"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 4;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "4 * 2 % 6 equals 2";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, twoReplacements)
{
    nlohmann::json j = {{"foo", "$FOO $BAR"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["FOO"] = std::string("foo");
    data["BAR"] = std::string("bar");

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "foo bar";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, twoReplacementsWithMath)
{
    nlohmann::json j = {{"foo", "4 / 2 equals $TEST / 2 $BAR"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["TEST"] = 4;
    data["BAR"] = std::string("bar");

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "4 / 2 equals 2 bar";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, hexAndWrongCase)
{
    nlohmann::json j = {{"Address", "0x54"},
                        {"Bus", 15},
                        {"Name", "$bus sensor 0"},
                        {"Type", "SomeType"}};

    boost::container::flat_map<std::string, BasicVariantType> data;
    data["BUS"] = 15;

    for (auto it = j.begin(); it != j.end(); it++)
    {
        templateCharReplace(it, data, 0);
    }
    nlohmann::json expected = {{"Address", 84},
                               {"Bus", 15},
                               {"Name", "15 sensor 0"},
                               {"Type", "SomeType"}};
    EXPECT_EQ(expected, j);
}

TEST(TemplateCharReplace, replaceSecondAsInt)
{
    nlohmann::json j = {{"foo", "twelve is $TEST"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;
    data["test"] = 12;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = "twelve is 12";
    EXPECT_EQ(expected, j["foo"]);
}

TEST(TemplateCharReplace, singleHex)
{
    nlohmann::json j = {{"foo", "0x54"}};
    auto it = j.begin();
    boost::container::flat_map<std::string, BasicVariantType> data;

    templateCharReplace(it, data, 0);

    nlohmann::json expected = 84;
    EXPECT_EQ(expected, j["foo"]);
}
