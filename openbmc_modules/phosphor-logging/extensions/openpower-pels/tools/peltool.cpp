/**
 * Copyright © 2019 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include "../bcd_time.hpp"
#include "../json_utils.hpp"
#include "../paths.hpp"
#include "../pel.hpp"
#include "../pel_types.hpp"
#include "../pel_values.hpp"

#include <CLI/CLI.hpp>
#include <bitset>
#include <fstream>
#include <iostream>
#include <phosphor-logging/log.hpp>
#include <regex>
#include <string>
#include <xyz/openbmc_project/Common/File/error.hpp>

namespace fs = std::filesystem;
using namespace phosphor::logging;
using namespace openpower::pels;
using sdbusplus::exception::SdBusError;
namespace file_error = sdbusplus::xyz::openbmc_project::Common::File::Error;
namespace message = openpower::pels::message;
namespace pv = openpower::pels::pel_values;

using PELFunc = std::function<void(const PEL&)>;
message::Registry registry(getPELReadOnlyDataPath() / message::registryFileName,
                           false);
namespace service
{
constexpr auto logging = "xyz.openbmc_project.Logging";
} // namespace service

namespace interface
{
constexpr auto deleteObj = "xyz.openbmc_project.Object.Delete";
constexpr auto deleteAll = "xyz.openbmc_project.Collection.DeleteAll";
} // namespace interface

namespace object_path
{
constexpr auto logEntry = "/xyz/openbmc_project/logging/entry/";
constexpr auto logging = "/xyz/openbmc_project/logging";
} // namespace object_path

/**
 * @brief helper function to get PEL commit timestamp from file name
 * @retrun BCDTime - PEL commit timestamp
 * @param[in] std::string - file name
 */
BCDTime fileNameToTimestamp(const std::string& fileName)
{
    std::string token = fileName.substr(0, fileName.find("_"));
    int i = 0;
    BCDTime tmp;
    if (token.length() >= 14)
    {
        try
        {
            tmp.yearMSB = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.yearLSB = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.month = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.day = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.hour = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.minutes = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.seconds = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
        i += 2;
        try
        {
            tmp.hundredths = std::stoi(token.substr(i, 2), 0, 16);
        }
        catch (std::exception& err)
        {
            std::cout << "Conversion failure: " << err.what() << std::endl;
        }
    }
    return tmp;
}

/**
 * @brief helper function to get PEL id from file name
 * @retrun uint32_t - PEL id
 * @param[in] std::string - file name
 */
uint32_t fileNameToPELId(const std::string& fileName)
{
    uint32_t num = 0;
    try
    {
        num = std::stoi(fileName.substr(fileName.find("_") + 1), 0, 16);
    }
    catch (std::exception& err)
    {
        std::cout << "Conversion failure: " << err.what() << std::endl;
    }
    return num;
}

/**
 * @brief helper function to check string suffix
 * @retrun bool - true with suffix matches
 * @param[in] std::string - string to check for suffix
 * @param[in] std::string - suffix string
 */
bool ends_with(const std::string& str, const std::string& end)
{
    size_t slen = str.size(), elen = end.size();
    if (slen < elen)
        return false;
    while (elen)
    {
        if (str[--slen] != end[--elen])
            return false;
    }
    return true;
}

/**
 * @brief get data form raw PEL file.
 * @param[in] std::string Name of file with raw PEL
 * @return std::vector<uint8_t> char vector read from raw PEL file.
 */
std::vector<uint8_t> getFileData(const std::string& name)
{
    std::ifstream file(name, std::ifstream::in);
    if (file.good())
    {
        std::vector<uint8_t> data{std::istreambuf_iterator<char>(file),
                                  std::istreambuf_iterator<char>()};
        return data;
    }
    else
    {
        return {};
    }
}

/**
 * @brief Creates JSON string of a PEL entry if fullPEL is false or prints to
 *        stdout the full PEL in JSON if fullPEL is true
 * @param[in] itr - std::map iterator of <uint32_t, BCDTime>
 * @param[in] hidden - Boolean to include hidden PELs
 * @param[in] includeInfo - Boolean to include informational PELs
 * @param[in] fullPEL - Boolean to print full JSON representation of PEL
 * @param[in] foundPEL - Boolean to check if any PEL is present
 * @param[in] scrubRegex - SRC regex object
 * @return std::string - JSON string of PEL entry (empty if fullPEL is true)
 */
template <typename T>
std::string genPELJSON(T itr, bool hidden, bool includeInfo, bool fullPEL,
                       bool& foundPEL,
                       const std::optional<std::regex>& scrubRegex)
{
    std::size_t found;
    std::string val;
    char tmpValStr[50];
    std::string listStr;
    char name[50];
    sprintf(name, "%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X_%.8X", itr.second.yearMSB,
            itr.second.yearLSB, itr.second.month, itr.second.day,
            itr.second.hour, itr.second.minutes, itr.second.seconds,
            itr.second.hundredths, itr.first);
    std::string fileName(name);
    fileName = EXTENSION_PERSIST_DIR "/pels/logs/" + fileName;
    try
    {
        std::vector<uint8_t> data = getFileData(fileName);
        if (data.empty())
        {
            log<level::ERR>("Empty PEL file",
                            entry("FILENAME=%s", fileName.c_str()));
            return listStr;
        }
        PEL pel{data};
        if (!pel.valid())
        {
            return listStr;
        }
        if (!includeInfo && pel.userHeader().severity() == 0)
        {
            return listStr;
        }
        std::bitset<16> actionFlags{pel.userHeader().actionFlags()};
        if (!hidden && actionFlags.test(hiddenFlagBit))
        {
            return listStr;
        }
        if (pel.primarySRC() && scrubRegex)
        {
            val = pel.primarySRC().value()->asciiString();
            if (std::regex_search(trimEnd(val), scrubRegex.value(),
                                  std::regex_constants::match_not_null))
            {
                return listStr;
            }
        }
        if (fullPEL)
        {
            if (!foundPEL)
            {
                std::cout << "[\n";
                foundPEL = true;
            }
            else
            {
                std::cout << ",\n\n";
            }
            pel.toJSON(registry);
        }
        else
        {
            // id
            listStr += "\t\"" +
                       getNumberString("0x%X", pel.privateHeader().id()) +
                       "\": {\n";
            // ASCII
            if (pel.primarySRC())
            {
                val = pel.primarySRC().value()->asciiString();
                listStr += "\t\t\"SRC\": \"" + trimEnd(val) + "\",\n";
                // Registry message
                auto regVal = pel.primarySRC().value()->getErrorDetails(
                    registry, DetailLevel::message, true);
                if (regVal)
                {
                    val = regVal.value();
                    listStr += "\t\t\"Message\": \"" + val + "\",\n";
                }
            }
            else
            {
                listStr += "\t\t\"SRC\": \"No SRC\",\n";
            }
            // platformid
            listStr += "\t\t\"PLID\": \"" +
                       getNumberString("0x%X", pel.privateHeader().plid()) +
                       "\",\n";
            // creatorid
            std::string creatorID =
                getNumberString("%c", pel.privateHeader().creatorID());
            val = pv::creatorIDs.count(creatorID) ? pv::creatorIDs.at(creatorID)
                                                  : "Unknown Creator ID";
            listStr += "\t\t\"CreatorID\": \"" + val + "\",\n";
            // subsytem
            std::string subsystem = pv::getValue(pel.userHeader().subsystem(),
                                                 pel_values::subsystemValues);
            listStr += "\t\t\"Subsystem\": \"" + subsystem + "\",\n";
            // commit time
            sprintf(tmpValStr, "%02X/%02X/%02X%02X %02X:%02X:%02X",
                    pel.privateHeader().commitTimestamp().month,
                    pel.privateHeader().commitTimestamp().day,
                    pel.privateHeader().commitTimestamp().yearMSB,
                    pel.privateHeader().commitTimestamp().yearLSB,
                    pel.privateHeader().commitTimestamp().hour,
                    pel.privateHeader().commitTimestamp().minutes,
                    pel.privateHeader().commitTimestamp().seconds);
            val = std::string(tmpValStr);
            listStr += "\t\t\"Commit Time\": \"" + val + "\",\n";
            // severity
            std::string severity = pv::getValue(pel.userHeader().severity(),
                                                pel_values::severityValues);
            listStr += "\t\t\"Sev\": \"" + severity + "\",\n ";
            // compID
            listStr += "\t\t\"CompID\": \"" +
                       getNumberString(
                           "0x%X", pel.privateHeader().header().componentID) +
                       "\",\n ";
            found = listStr.rfind(",");
            if (found != std::string::npos)
            {
                listStr.replace(found, 1, "");
                listStr += "\t},\n";
            }
            foundPEL = true;
        }
    }
    catch (std::exception& e)
    {
        log<level::ERR>("Hit exception while reading PEL File",
                        entry("FILENAME=%s", fileName.c_str()),
                        entry("ERROR=%s", e.what()));
    }
    return listStr;
}

/**
 * @brief Print a list of PELs or a JSON array of PELs
 * @param[in] order - Boolean to print in reverse orser
 * @param[in] hidden - Boolean to include hidden PELs
 * @param[in] includeInfo - Boolean to include informational PELs
 * @param[in] fullPEL - Boolean to print full PEL into a JSON array
 * @param[in] scrubRegex - SRC regex object
 */
void printPELs(bool order, bool hidden, bool includeInfo, bool fullPEL,
               const std::optional<std::regex>& scrubRegex)
{
    std::string listStr;
    std::map<uint32_t, BCDTime> PELs;
    listStr = "{\n";
    for (auto it = fs::directory_iterator(EXTENSION_PERSIST_DIR "/pels/logs");
         it != fs::directory_iterator(); ++it)
    {
        if (!fs::is_regular_file((*it).path()))
        {
            continue;
        }
        else
        {
            PELs.emplace(fileNameToPELId((*it).path().filename()),
                         fileNameToTimestamp((*it).path().filename()));
        }
    }
    bool foundPEL = false;
    auto buildJSON = [&listStr, &hidden, &includeInfo, &fullPEL, &foundPEL,
                      &scrubRegex](const auto& i) {
        listStr +=
            genPELJSON(i, hidden, includeInfo, fullPEL, foundPEL, scrubRegex);
    };
    if (order)
    {
        std::for_each(PELs.rbegin(), PELs.rend(), buildJSON);
    }
    else
    {
        std::for_each(PELs.begin(), PELs.end(), buildJSON);
    }

    if (foundPEL)
    {
        if (fullPEL)
        {
            std::cout << "]" << std::endl;
        }
        else
        {
            std::size_t found;
            found = listStr.rfind(",");
            if (found != std::string::npos)
            {
                listStr.replace(found, 1, "");
                listStr += "}\n";
                printf("%s", listStr.c_str());
            }
        }
    }
    else
    {
        std::string emptyJSON = fullPEL ? "[]" : "{}";
        std::cout << emptyJSON << std::endl;
    }
}

/**
 * @brief Calls the function passed in on the PEL with the ID
 *        passed in.
 *
 * @param[in] id - The string version of the PEL ID, either with or
 *                 without the 0x prefix.
 * @param[in] func - The std::function<void(const PEL&)> function to run.
 */
void callFunctionOnPEL(const std::string& id, const PELFunc& func)
{
    std::string pelID{id};
    std::transform(pelID.begin(), pelID.end(), pelID.begin(), toupper);

    if (pelID.find("0X") == 0)
    {
        pelID.erase(0, 2);
    }

    bool found = false;

    for (auto it = fs::directory_iterator(EXTENSION_PERSIST_DIR "/pels/logs");
         it != fs::directory_iterator(); ++it)
    {
        // The PEL ID is part of the filename, so use that to find the PEL.

        if (!fs::is_regular_file((*it).path()))
        {
            continue;
        }

        if (ends_with((*it).path(), pelID))
        {
            found = true;

            auto data = getFileData((*it).path());
            if (!data.empty())
            {
                PEL pel{data};

                try
                {
                    func(pel);
                }
                catch (std::exception& e)
                {
                    std::cerr
                        << " Internal function threw an exception: " << e.what()
                        << "\n";
                    exit(1);
                }
            }
            else
            {
                std::cerr << "Could not read PEL file\n";
                exit(1);
            }
            break;
        }
    }

    if (!found)
    {
        std::cerr << "PEL not found\n";
        exit(1);
    }
}

/**
 * @brief Delete a PEL by deleting its corresponding event log.
 *
 * @param[in] pel - The PEL to delete
 */
void deletePEL(const PEL& pel)
{
    std::string path{object_path::logEntry};
    path += std::to_string(pel.obmcLogID());

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(service::logging, path.c_str(),
                                          interface::deleteObj, "Delete");
        auto reply = bus.call(method);
    }
    catch (const SdBusError& e)
    {
        std::cerr << "D-Bus call to delete event log " << pel.obmcLogID()
                  << " failed: " << e.what() << "\n";
        exit(1);
    }
}

/**
 * @brief Delete all PELs by deleting all event logs.
 */
void deleteAllPELs()
{
    try
    {
        // This may move to an audit log some day
        log<level::INFO>("peltool deleting all event logs");

        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(service::logging, object_path::logging,
                                interface::deleteAll, "DeleteAll");
        auto reply = bus.call(method);
    }
    catch (const SdBusError& e)
    {
        std::cerr << "D-Bus call to delete all event logs failed: " << e.what()
                  << "\n";
        exit(1);
    }
}

/**
 * @brief Display a single PEL
 *
 * @param[in] pel - the PEL to display
 */
void displayPEL(const PEL& pel)
{
    if (pel.valid())
    {
        pel.toJSON(registry);
    }
    else
    {
        std::cerr << "PEL was malformed\n";
        exit(1);
    }
}

/**
 * @brief Print number of PELs
 * @param[in] hidden - Bool to include hidden logs
 * @param[in] includeInfo - Bool to include informational logs
 * @param[in] scrubRegex - SRC regex object
 */
void printPELCount(bool hidden, bool includeInfo,
                   const std::optional<std::regex>& scrubRegex)
{
    std::size_t count = 0;
    for (auto it = fs::directory_iterator(EXTENSION_PERSIST_DIR "/pels/logs");
         it != fs::directory_iterator(); ++it)
    {
        if (!fs::is_regular_file((*it).path()))
        {
            continue;
        }
        std::vector<uint8_t> data = getFileData((*it).path());
        if (data.empty())
        {
            continue;
        }
        PEL pel{data};
        if (!pel.valid())
        {
            continue;
        }
        if (!includeInfo && pel.userHeader().severity() == 0)
        {
            continue;
        }
        std::bitset<16> actionFlags{pel.userHeader().actionFlags()};
        if (!hidden && actionFlags.test(hiddenFlagBit))
        {
            continue;
        }
        if (pel.primarySRC() && scrubRegex)
        {
            std::string val = pel.primarySRC().value()->asciiString();
            if (std::regex_search(trimEnd(val), scrubRegex.value(),
                                  std::regex_constants::match_not_null))
            {
                continue;
            }
        }
        count++;
    }
    std::cout << "{\n"
              << "    \"Number of PELs found\": "
              << getNumberString("%d", count) << "\n}\n";
}

/**
 * @brief Generate regex pattern object from file contents
 * @param[in] scrubFile - File containing regex pattern
 * @return std::regex - SRC regex object
 */
std::regex genRegex(std::string& scrubFile)
{
    std::string pattern;
    std::ifstream contents(scrubFile);
    if (contents.fail())
    {
        std::cerr << "Can't open \"" << scrubFile << "\"\n";
        exit(1);
    }
    std::string line;
    while (std::getline(contents, line))
    {
        if (!line.empty())
        {
            pattern.append(line + "|");
        }
    }
    try
    {
        std::regex scrubRegex(pattern, std::regex::icase);
        return scrubRegex;
    }
    catch (std::regex_error& e)
    {
        if (e.code() == std::regex_constants::error_collate)
            std::cerr << "Invalid collating element request\n";
        else if (e.code() == std::regex_constants::error_ctype)
            std::cerr << "Invalid character class\n";
        else if (e.code() == std::regex_constants::error_escape)
            std::cerr << "Invalid escape character or trailing escape\n";
        else if (e.code() == std::regex_constants::error_backref)
            std::cerr << "Invalid back reference\n";
        else if (e.code() == std::regex_constants::error_brack)
            std::cerr << "Mismatched bracket ([ or ])\n";
        else if (e.code() == std::regex_constants::error_paren)
        {
            // to catch return code error_badrepeat when error_paren is retured
            // instead
            size_t pos = pattern.find_first_of("*+?{");
            while (pos != std::string::npos)
            {
                if (pos == 0 || pattern.substr(pos - 1, 1) == "|")
                {
                    std::cerr
                        << "A repetition character (*, ?, +, or {) was not "
                           "preceded by a valid regular expression\n";
                    exit(1);
                }
                pos = pattern.find_first_of("*+?{", pos + 1);
            }
            std::cerr << "Mismatched parentheses (( or ))\n";
        }
        else if (e.code() == std::regex_constants::error_brace)
            std::cerr << "Mismatched brace ({ or })\n";
        else if (e.code() == std::regex_constants::error_badbrace)
            std::cerr << "Invalid range inside a { }\n";
        else if (e.code() == std::regex_constants::error_range)
            std::cerr << "Invalid character range (e.g., [z-a])\n";
        else if (e.code() == std::regex_constants::error_space)
            std::cerr << "Insufficient memory to handle regular expression\n";
        else if (e.code() == std::regex_constants::error_badrepeat)
            std::cerr << "A repetition character (*, ?, +, or {) was not "
                         "preceded by a valid regular expression\n";
        else if (e.code() == std::regex_constants::error_complexity)
            std::cerr << "The requested match is too complex\n";
        else if (e.code() == std::regex_constants::error_stack)
            std::cerr << "Insufficient memory to evaluate a match\n";
        exit(1);
    }
}

static void exitWithError(const std::string& help, const char* err)
{
    std::cerr << "ERROR: " << err << std::endl << help << std::endl;
    exit(-1);
}

int main(int argc, char** argv)
{
    CLI::App app{"OpenBMC PEL Tool"};
    std::string fileName;
    std::string idPEL;
    std::string idToDelete;
    std::string scrubFile;
    std::optional<std::regex> scrubRegex;
    bool listPEL = false;
    bool listPELDescOrd = false;
    bool hidden = false;
    bool includeInfo = false;
    bool deleteAll = false;
    bool showPELCount = false;
    bool fullPEL = false;

    app.set_help_flag("--help", "Print this help message and exit");
    app.add_option("--file", fileName, "Display a PEL using its Raw PEL file");
    app.add_option("-i, --id", idPEL, "Display a PEL based on its ID");
    app.add_flag("-a", fullPEL, "Display all PELs");
    app.add_flag("-l", listPEL, "List PELs");
    app.add_flag("-n", showPELCount, "Show number of PELs");
    app.add_flag("-r", listPELDescOrd, "Reverse order of output");
    app.add_flag("-h", hidden, "Include hidden PELs");
    app.add_flag("-f,--info", includeInfo, "Include informational PELs");
    app.add_option("-d, --delete", idToDelete, "Delete a PEL based on its ID");
    app.add_flag("-D, --delete-all", deleteAll, "Delete all PELs");
    app.add_option("-s, --scrub", scrubFile,
                   "File containing SRC regular expressions to ignore");

    CLI11_PARSE(app, argc, argv);

    if (!fileName.empty())
    {
        std::vector<uint8_t> data = getFileData(fileName);
        if (!data.empty())
        {
            PEL pel{data};
            pel.toJSON(registry);
        }
        else
        {
            exitWithError(app.help("", CLI::AppFormatMode::All),
                          "Raw PEL file can't be read.");
        }
    }
    else if (!idPEL.empty())
    {
        callFunctionOnPEL(idPEL, displayPEL);
    }
    else if (fullPEL || listPEL)
    {
        if (!scrubFile.empty())
        {
            scrubRegex = genRegex(scrubFile);
        }
        printPELs(listPELDescOrd, hidden, includeInfo, fullPEL, scrubRegex);
    }
    else if (showPELCount)
    {
        if (!scrubFile.empty())
        {
            scrubRegex = genRegex(scrubFile);
        }
        printPELCount(hidden, includeInfo, scrubRegex);
    }
    else if (!idToDelete.empty())
    {
        callFunctionOnPEL(idToDelete, deletePEL);
    }
    else if (deleteAll)
    {
        deleteAllPELs();
    }
    else
    {
        std::cout << app.help("", CLI::AppFormatMode::All) << std::endl;
    }
    return 0;
}
