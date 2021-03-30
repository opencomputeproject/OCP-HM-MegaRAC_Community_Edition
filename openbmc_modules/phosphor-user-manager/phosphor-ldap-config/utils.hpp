#pragma once

#include <string>

namespace phosphor
{
namespace ldap
{

/** @brief checks that the given URI is valid LDAP's URI.
 *      LDAP's URL begins with "ldap://" and LDAPS's URL begins with "ldap://"
 *  @param[in] URI - URI which needs to be validated.
 *  @param[in] scheme - LDAP's scheme, scheme equals to "ldaps" to validate
 *       against LDAPS type URI, for LDAP type URI it is equals to "ldap".
 *  @returns true if it is valid otherwise false.
 */
bool isValidLDAPURI(const std::string& URI, const char* scheme);

} // namespace ldap
} // namespace phosphor
