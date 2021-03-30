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
#include "extensions/openpower-pels/paths.hpp"

#include <filesystem>

namespace openpower
{
namespace pels
{

// Use paths that work in unit tests.

std::filesystem::path getPELIDFile()
{
    static std::string idFile;

    if (idFile.empty())
    {
        char templ[] = "/tmp/logidtestXXXXXX";
        std::filesystem::path dir = mkdtemp(templ);
        idFile = dir / "logid";
    }
    return idFile;
}

std::filesystem::path getPELRepoPath()
{
    static std::string repoPath;

    if (repoPath.empty())
    {
        char templ[] = "/tmp/repopathtestXXXXXX";
        std::filesystem::path dir = mkdtemp(templ);
        repoPath = dir;
    }
    return repoPath;
}

std::filesystem::path getPELReadOnlyDataPath()
{
    static std::string dataPath;

    if (dataPath.empty())
    {
        char templ[] = "/tmp/pelrodatatestXXXXXX";
        dataPath = mkdtemp(templ);
    }

    return dataPath;
}

size_t getPELRepoSize()
{
    // 100KB
    return 100 * 1024;
}

size_t getMaxNumPELs()
{
    return 100;
}

} // namespace pels
} // namespace openpower
