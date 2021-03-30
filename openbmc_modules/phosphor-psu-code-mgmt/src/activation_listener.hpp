#pragma once

#include <string>

class ActivationListener
{
  public:
    virtual ~ActivationListener() = default;

    /** @brief Notify a PSU is updated
     *
     * @param[in]  versionId - The versionId of the activation
     * @param[in]  psuInventoryPath - The PSU inventory path that is updated
     */
    virtual void onUpdateDone(const std::string& versionId,
                              const std::string& psuInventoryPath) = 0;
};
