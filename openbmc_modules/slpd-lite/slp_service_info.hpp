#pragma once

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace slp
{
struct ConfigData
{
    std::string name;
    std::string type;
    std::string port;

    friend std::istream& operator>>(std::istream& str, ConfigData& data)
    {
        std::string line;
        constexpr auto DELIMITER = " ";
        size_t delimtrPos = 0;
        size_t delimtrPrevPos = 0;
        std::array<std::string, 3> tokens;
        std::getline(str, line);
        size_t count = 0;

        delimtrPos = line.find(DELIMITER, delimtrPrevPos);
        while (delimtrPos != std::string::npos)
        {
            tokens[count] =
                line.substr(delimtrPrevPos, (delimtrPos - delimtrPrevPos));
            delimtrPrevPos = delimtrPos + 1;

            delimtrPos = line.find(DELIMITER, delimtrPrevPos);
            if (delimtrPos == std::string::npos &&
                delimtrPrevPos < line.length())
            {
                delimtrPos = line.length();
            }

            count++;
        }

        if (count > 2)
        {
            data.name = tokens[0];
            data.type = tokens[1];
            data.port = tokens[2];
        }
        else
        {
            str.setstate(std::ios::failbit);
        }
        return str;
    }
};
} // namespace slp
