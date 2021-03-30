#pragma once

#include "internal/sys.hpp"

#include <gmock/gmock.h>

namespace blobs
{
namespace internal
{

class InternalDlSysMock : public DlSysInterface
{
  public:
    virtual ~InternalDlSysMock() = default;

    MOCK_CONST_METHOD0(dlerror, const char*());
    MOCK_CONST_METHOD2(dlopen, void*(const char*, int));
    MOCK_CONST_METHOD2(dlsym, void*(void*, const char*));
};

} // namespace internal
} // namespace blobs
