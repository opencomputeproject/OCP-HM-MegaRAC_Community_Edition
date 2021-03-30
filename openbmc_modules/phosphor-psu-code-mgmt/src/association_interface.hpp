#pragma once

#include <string>

class AssociationInterface
{
  public:
    virtual ~AssociationInterface() = default;

    /** @brief Create an active association to the
     *  newly active software image
     *
     * @param[in]  path - The path to create the association to.
     */
    virtual void createActiveAssociation(const std::string& path) = 0;

    /** @brief Add the functional association to the
     *  new "running" PSU images
     *
     * @param[in]  path - The path to add the association to.
     */
    virtual void addFunctionalAssociation(const std::string& path) = 0;

    /** @brief Add the updateable association to the
     *  "running" PSU software image
     *
     * @param[in]  path - The path to create the association.
     */
    virtual void addUpdateableAssociation(const std::string& path) = 0;

    /** @brief Remove the associations from the provided software image path
     *
     * @param[in]  path - The path to remove the association from.
     */
    virtual void removeAssociation(const std::string& path) = 0;
};
