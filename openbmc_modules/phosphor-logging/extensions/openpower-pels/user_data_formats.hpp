#pragma once

namespace openpower
{
namespace pels
{

enum class UserDataFormat
{
    json = 1,
    cbor = 2,
    text = 3,
    custom = 4
};

enum class UserDataFormatVersion
{
    json = 1,
    cbor = 1,
    text = 1
};

} // namespace pels
} // namespace openpower
