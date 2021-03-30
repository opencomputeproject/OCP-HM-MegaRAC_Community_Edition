#pragma once

#include <cstdint>
#include <string>

namespace ethstats
{

class EthStatsInterface
{
  public:
    virtual ~EthStatsInterface() = default;

    /** Given an ifname and a statistic, validate both.
     *
     * @param[in] path - the interface name and statistics field
     * @return true if both valid, false otherwise.
     */
    virtual bool validIfNameAndField(const std::string& path) const = 0;

    /** Given an ifname and a statistic, return the value.
     *
     * @param[in] path - the interface name and statistics field
     * @return the value of that statistic for that interface.
     */
    virtual std::uint64_t readStatistic(const std::string& path) const = 0;
};

class EthStats : public EthStatsInterface
{
  public:
    EthStats() = default;
    ~EthStats() = default;

    bool validIfNameAndField(const std::string& path) const override;
    std::uint64_t readStatistic(const std::string& path) const override;
};

} // namespace ethstats
