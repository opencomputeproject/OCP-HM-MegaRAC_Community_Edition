/*
 * Copyright 2018 Google Inc.
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

#include "sys.hpp"

#include <dlfcn.h>

namespace blobs
{

namespace internal
{

const char* DlSysImpl::dlerror() const
{
    return ::dlerror();
}

void* DlSysImpl::dlopen(const char* filename, int flags) const
{
    return ::dlopen(filename, flags);
}

void* DlSysImpl::dlsym(void* handle, const char* symbol) const
{
    return ::dlsym(handle, symbol);
}

DlSysImpl dlsys_impl;

} // namespace internal
} // namespace blobs
