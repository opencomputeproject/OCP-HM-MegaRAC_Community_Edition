#pragma once

#include "sensorset.hpp"

#include <fstream>
#include <string>

namespace env
{

/** @class Env
 *  @brief Overridable std::getenv interface
 */
struct Env
{
    virtual ~Env() = default;

    virtual const char* get(const char* key) const = 0;
};

/** @class EnvImpl
 *  @brief Concrete implementation that calls std::getenv
 */
struct EnvImpl : public Env
{
    const char* get(const char* key) const override;
};

/** @brief Default instantiation of Env */
extern EnvImpl env_impl;

/** @brief Reads an environment variable
 *
 *  Reads the environment for that key
 *
 *  @param[in] key - the key
 *  @param[in] env - env interface that defaults to calling std::getenv
 *
 *  @return string - the env var value
 */
inline std::string getEnv(const char* key, const Env* env = &env_impl)
{
    // Be aware value could be nullptr
    auto value = env->get(key);
    return (value) ? std::string(value) : std::string();
}

/** @brief Reads an environment variable
 *
 *  Reads <prefix>_<sensor.first><sensor.second>
 *
 *  @param[in] prefix - the variable prefix
 *  @param[in] sensor - Sensor details
 *  @param[in] env - env interface that defaults to calling std::getenv
 *
 *  @return string - the env var value
 */
inline std::string getEnv(const char* prefix, const SensorSet::key_type& sensor,
                          const Env* env = &env_impl)
{
    std::string key;

    key.assign(prefix);
    key.append(1, '_');
    key.append(sensor.first);
    key.append(sensor.second);

    return getEnv(key.c_str(), env);
}

/** @brief Reads an environment variable, and takes type and id separately
 *
 *  @param[in] prefix - the variable prefix
 *  @param[in] type - sensor type, like 'temp'
 *  @param[in] id - sensor ID, like '5'
 *  @param[in] env - env interface that defaults to calling std::getenv
 *
 *  @return string - the env var value
 */
inline std::string getEnv(const char* prefix, const std::string& type,
                          const std::string& id, const Env* env = &env_impl)
{
    SensorSet::key_type sensor{type, id};
    return getEnv(prefix, sensor, env);
}

/** @brief Gets the ID for the sensor with a level of indirection
 *
 *  Read the ID from the <path>/<item><X>_<suffix> file.
 *  <item> & <X> are populated from the sensor key.
 *
 *  @param[in] path - Directory path of the label file
 *  @param[in] fileSuffix - The file suffix
 *  @param[in] sensor - Sensor details
 */
inline std::string getIndirectID(std::string path,
                                 const std::string& fileSuffix,
                                 const SensorSet::key_type& sensor)
{
    std::string content;

    path.append(sensor.first);
    path.append(sensor.second);
    path.append(1, '_');
    path.append(fileSuffix);

    std::ifstream handle(path.c_str());
    if (!handle.fail())
    {
        content.assign((std::istreambuf_iterator<char>(handle)),
                       (std::istreambuf_iterator<char>()));

        if (!content.empty())
        {
            // remove the newline
            content.pop_back();
        }
    }

    return content;
}

} // namespace env
