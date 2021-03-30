#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <tuple>

/**
 * Key-value store for string-type system info parameters.
 */
class SysInfoParamStoreIntf
{
  public:
    virtual ~SysInfoParamStoreIntf()
    {
    }

    /**
     * Returns true if parameter is found. If and only if s is non-null,
     * invokes the parameter's callback and writes the value.
     *
     * @param[in] paramSelector - the key to lookup.
     * @return tuple of bool and string, true if parameter is found and
     * string set accordingly.
     */
    virtual std::tuple<bool, std::string>
        lookup(uint8_t paramSelector) const = 0;

    /**
     * Update a parameter by its code with a string value.
     *
     * @param[in] paramSelector - the key to update.
     * @param[in] s - the value to set.
     */
    virtual void update(uint8_t paramSelector, const std::string& s) = 0;

    /**
     * Update a parameter by its code with a callback that is called to retrieve
     * its value whenever called. Callback must be idempotent, as it may be
     * called multiple times by the host to retrieve the parameter by chunks.
     *
     * @param[in] paramSelector - the key to update.
     * @param[in] callback - the callback to use for parameter retrieval.
     */
    virtual void update(uint8_t paramSelector,
                        const std::function<std::string()>& callback) = 0;

    // TODO: Store "read-only" flag for each parameter.
    // TODO: Function to erase a parameter?
};

/**
 * Implement the system info parameters store as a map of callbacks.
 */
class SysInfoParamStore : public SysInfoParamStoreIntf
{
  public:
    std::tuple<bool, std::string> lookup(uint8_t paramSelector) const override;
    void update(uint8_t paramSelector, const std::string& s) override;
    void update(uint8_t paramSelector,
                const std::function<std::string()>& callback) override;

  private:
    std::map<uint8_t, std::function<std::string()>> params;
};
