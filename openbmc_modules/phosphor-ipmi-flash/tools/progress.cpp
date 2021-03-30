/*
 * Copyright 2019 Google Inc.
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

#include "progress.hpp"

#include <cstdio>

namespace host_tool
{

void ProgressStdoutIndicator::updateProgress(std::int64_t bytes)
{
    /* Print progress update. */
    currentBytes += bytes;
    std::fprintf(stdout, "\rProgress: %.2f%%",
                 100.0 * currentBytes / totalBytes);
    if (currentBytes == totalBytes)
        std::fprintf(stdout, "\n");

    std::fflush(stdout);
}

void ProgressStdoutIndicator::start(std::int64_t bytes)
{
    totalBytes = bytes;
    currentBytes = 0;
}

} // namespace host_tool
