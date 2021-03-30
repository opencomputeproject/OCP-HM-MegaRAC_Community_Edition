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
#include "json_utils.hpp"

#include <stdio.h>

#include <cstring>
#include <sstream>
#include <string>

namespace openpower
{
namespace pels
{

std::string escapeJSON(const std::string& input)
{
    std::string output;
    output.reserve(input.length());

    for (const auto c : input)
    {
        switch (c)
        {
            case '"':
                output += "\\\"";
                break;
            case '/':
                output += "\\/";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            case '\\':
                output += "\\\\";
                break;
            default:
                output += c;
                break;
        }
    }

    return output;
}
char* dumpHex(const void* data, size_t size, size_t indentCount)
{
    const int symbolSize = 100;
    std::string jsonIndent(indentLevel * indentCount, 0x20);
    jsonIndent.append("\"");
    char* buffer = (char*)calloc(std::max(70, 10 * (int)size), sizeof(char));
    char* symbol = (char*)calloc(symbolSize, sizeof(char));
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i)
    {
        if (i % 16 == 0)
        {
            strcat(buffer, jsonIndent.c_str());
        }
        snprintf(symbol, symbolSize, "%02X ", ((unsigned char*)data)[i]);
        strcat(buffer, symbol);
        memset(symbol, 0, strlen(symbol));
        if (((unsigned char*)data)[i] >= ' ' &&
            ((unsigned char*)data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char*)data)[i];
        }
        else
        {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            std::string asciiString(ascii);
            asciiString = escapeJSON(asciiString);
            const char* asciiToPrint = asciiString.c_str();
            strcat(buffer, " ");
            if ((i + 1) % 16 == 0)
            {
                if (i + 1 != size)
                {
                    snprintf(symbol, symbolSize, "|  %s\",\n", asciiToPrint);
                }
                else
                {
                    snprintf(symbol, symbolSize, "|  %s\"\n", asciiToPrint);
                }
                strcat(buffer, symbol);
                memset(symbol, 0, strlen(symbol));
            }
            else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    strcat(buffer, " ");
                }
                for (j = (i + 1) % 16; j < 16; ++j)
                {
                    strcat(buffer, "   ");
                }
                std::string asciiString2(ascii);
                asciiString2 = escapeJSON(asciiString2);
                asciiToPrint = asciiString2.c_str();
                snprintf(symbol, symbolSize, "|  %s\"\n", asciiToPrint);
                strcat(buffer, symbol);
                memset(symbol, 0, strlen(symbol));
            }
        }
    }
    free(symbol);
    return buffer;
}

void jsonInsert(std::string& jsonStr, const std::string& fieldName,
                std::string fieldValue, uint8_t indentCount)
{
    const int8_t spacesToAppend =
        colAlign - (indentCount * indentLevel) - fieldName.length() - 3;
    const std::string jsonIndent(indentCount * indentLevel, 0x20);
    jsonStr.append(jsonIndent + "\"" + fieldName + "\":");
    if (spacesToAppend >= 0)
    {
        jsonStr.append(spacesToAppend, 0x20);
    }
    else
    {
        jsonStr.append(1, 0x20);
    }
    jsonStr.append("\"" + fieldValue + "\",\n");
}

void jsonInsertArray(std::string& jsonStr, const std::string& fieldName,
                     std::vector<std::string>& values, uint8_t indentCount)
{
    const std::string jsonIndent(indentCount * indentLevel, 0x20);
    if (!values.empty())
    {
        jsonStr.append(jsonIndent + "\"" + fieldName + "\": [\n");
        for (size_t i = 0; i < values.size(); i++)
        {
            jsonStr.append(colAlign, 0x20);
            if (i == values.size() - 1)
            {
                jsonStr.append("\"" + values[i] + "\"\n");
            }
            else
            {
                jsonStr.append("\"" + values[i] + "\",\n");
            }
        }
        jsonStr.append(jsonIndent + "],\n");
    }
    else
    {
        const int8_t spacesToAppend =
            colAlign - (indentCount * indentLevel) - fieldName.length() - 3;
        jsonStr.append(jsonIndent + "\"" + fieldName + "\":");
        if (spacesToAppend > 0)
        {
            jsonStr.append(spacesToAppend, 0x20);
        }
        else
        {
            jsonStr.append(1, 0x20);
        }
        jsonStr.append("[],\n");
    }
}

std::string trimEnd(std::string s)
{
    const char* t = " \t\n\r\f\v";
    if (s.find_last_not_of(t) != std::string::npos)
    {
        s.erase(s.find_last_not_of(t) + 1);
    }
    return s;
}
} // namespace pels
} // namespace openpower
