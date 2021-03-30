#pragma once
#include <sstream>
#include <string>

namespace witherspoon
{
namespace power
{
namespace util
{

/**
 * @class NamesValues
 *
 * Builds up a string of name=value pairs for use in
 * situations like error log metadata.
 *
 * Names and values can be added to an instance of this
 * class, and then calling get() will return a string
 * that appends them all together.
 *
 * Currently, just uint64_t values that will be displayed
 * in hex are supported.
 *
 * For example:
 *     NamesValues m;
 *     uint8_t d = 0xFF;
 *
 *     m.add("STATUS_VOUT", d);
 *     m.add("STATUS_WORD", 0xABCD);
 *
 *     m.get() //returns: "STATUS_VOUT=0xFF|STATUS_WORD=0xABCD"
 */
class NamesValues
{
  public:
    NamesValues() = default;
    NamesValues(const NamesValues&) = default;
    NamesValues& operator=(const NamesValues&) = default;
    NamesValues(NamesValues&&) = default;
    NamesValues& operator=(NamesValues&&) = default;

    /**
     * Adds a name/value pair to the object
     *
     * @param name - the name to add
     * @param value - the value to add
     */
    void add(const std::string& name, uint64_t value)
    {
        addDivider();
        addName(name);
        addValue(value);
    }

    /**
     * Returns a formatted concatenation of all of the names and
     * their values.
     *
     * @return string - "<name1>=<value1>|<name2>=<value2>..etc"
     */
    const std::string& get() const
    {
        return all;
    }

  private:
    /**
     * Adds a name to the object
     *
     * @param name - the name to add
     */
    void addName(const std::string& name)
    {
        all += name + '=';
    }

    /**
     * Adds a value to the object
     *
     * @param value - the value to add
     */
    void addValue(uint64_t value)
    {
        std::ostringstream stream;
        stream << "0x" << std::hex << value;
        all += stream.str();
    }

    /**
     * Adds a divider to the summary string to
     * break up the name/value pairs
     */
    void addDivider()
    {
        if (!all.empty())
        {
            all += '|';
        }
    }

    /**
     * The string containing all name/value pairs
     */
    std::string all;
};

} // namespace util
} // namespace power
} // namespace witherspoon
