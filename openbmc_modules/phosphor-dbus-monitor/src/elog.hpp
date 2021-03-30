#pragma once
#include "callback.hpp"

#include <experimental/tuple>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <sdbusplus/exception.hpp>
#include <string>

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

/** @struct ToString
 * @brief Convert numbers to strings
 */
template <typename T>
struct ToString
{
    static auto op(T&& value)
    {
        return std::to_string(std::forward<T>(value));
    }
};

template <>
struct ToString<std::string>
{
    static auto op(const std::string& value)
    {
        return value;
    }
};

/** @class ElogBase
 *  @brief Elog callback implementation.
 *
 *  The elog callback logs the elog and
 *  elog metadata.
 */
class ElogBase : public Callback
{
  public:
    ElogBase(const ElogBase&) = delete;
    ElogBase(ElogBase&&) = default;
    ElogBase& operator=(const ElogBase&) = delete;
    ElogBase& operator=(ElogBase&&) = default;
    virtual ~ElogBase() = default;
    ElogBase() : Callback()
    {
    }

    /** @brief Callback interface implementation. */
    void operator()(Context ctx) override;

  private:
    /** @brief Delegate type specific calls to subclasses. */
    virtual void log() const = 0;
};

namespace detail
{

/** @class CallElog
 *  @brief Provide explicit call forwarding to phosphor::logging::report.
 *
 *  @tparam T - Error log type
 *  @tparam Args - Metadata fields types.
 */
template <typename T, typename... Args>
struct CallElog
{
    static void op(Args&&... args)
    {
        phosphor::logging::report<T>(std::forward<Args>(args)...);
    }
};

} // namespace detail

/** @class Elog
 *  @brief C++ type specific logic for the elog callback.
 *         The elog callback logs the elog and elog metadata.
 *
 *  @tparam T - Error log type
 *  @tparam Args - Metadata fields types.
 *  @param[in] arguments - Metadata fields to be added to the error log
 */
template <typename T, typename... Args>
class Elog : public ElogBase
{
  public:
    Elog(const Elog&) = delete;
    Elog(Elog&&) = default;
    Elog& operator=(const Elog&) = delete;
    Elog& operator=(Elog&&) = default;
    ~Elog() = default;
    Elog(Args&&... arguments) :
        ElogBase(), args(std::forward<Args>(arguments)...)
    {
    }

  private:
    /** @brief elog interface implementation. */
    void log() const override
    {
        std::experimental::apply(detail::CallElog<T, Args...>::op,
                                 std::tuple_cat(args));
    }
    std::tuple<Args...> args;
};

/**
 * @class ElogWithMetadataCapture
 *
 * @brief A callback class that will save the paths, names, and
 *       current values of certain properties in the metadata of the
 *       error log it creates.
 *
 * The intended use case of this class is to create an error log with
 * metadata that includes the property names and values that caused
 * the condition to issue this callback.  When the condition ran, it had
 * set the pass/fail field on each property it checked in the properties'
 * entries in the Storage array.  This class then looks at those pass/fail
 * fields to see which properties to log.
 *
 * Note that it's OK if different conditions and callbacks share the same
 * properties because everything runs serially, so another condition can't
 * touch those pass/fail fields until all of the first condition's callbacks
 * are done.
 *
 * This class requires that the error log created only have 1 metadata field,
 * and it must take a string.
 *
 * @tparam errorType - Error log type
 * @tparam metadataType - The metadata to use
 * @tparam propertyType - The data type of the captured properties
 */
template <typename errorType, typename metadataType, typename propertyType>
class ElogWithMetadataCapture : public IndexedCallback
{
  public:
    ElogWithMetadataCapture() = delete;
    ElogWithMetadataCapture(const ElogWithMetadataCapture&) = delete;
    ElogWithMetadataCapture(ElogWithMetadataCapture&&) = default;
    ElogWithMetadataCapture& operator=(const ElogWithMetadataCapture&) = delete;
    ElogWithMetadataCapture& operator=(ElogWithMetadataCapture&&) = default;
    virtual ~ElogWithMetadataCapture() = default;
    explicit ElogWithMetadataCapture(const PropertyIndex& index) :
        IndexedCallback(index)
    {
    }

    /**
     * @brief Callback interface implementation that
     *        creates an error log
     */
    void operator()(Context ctx) override
    {
        if (ctx == Context::START)
        {
            // No action should be taken as this call back is being called from
            // daemon Startup.
            return;
        }
        auto data = captureMetadata();

        phosphor::logging::report<errorType>(metadataType(data.c_str()));
    }

  private:
    /**
     * @brief Builds a metadata string with property information
     *
     * Finds all of the properties in the index that have
     * their condition pass/fail fields (get<resultIndex>(storage))
     * set to true, and then packs those paths, names, and values
     * into a metadata string that looks like:
     *
     * |path1:name1=value1|path2:name2=value2|...
     *
     * @return The metadata string
     */
    std::string captureMetadata()
    {
        std::string metadata{'|'};

        for (const auto& n : index)
        {
            const auto& storage = std::get<storageIndex>(n.second).get();
            const auto& result = std::get<resultIndex>(storage);

            if (!result.empty() && any_ns::any_cast<bool>(result))
            {
                const auto& path = std::get<pathIndex>(n.first).get();
                const auto& propertyName =
                    std::get<propertyIndex>(n.first).get();
                auto value =
                    ToString<propertyType>::op(any_ns::any_cast<propertyType>(
                        std::get<valueIndex>(storage)));

                metadata += path + ":" + propertyName + '=' + value + '|';
            }
        }

        return metadata;
    };
};

/** @brief Argument type deduction for constructing Elog instances.
 *
 *  @tparam T - Error log type
 *  @tparam Args - Metadata fields types.
 *  @param[in] arguments - Metadata fields to be added to the error log
 */
template <typename T, typename... Args>
auto makeElog(Args&&... arguments)
{
    return std::make_unique<Elog<T, Args...>>(std::forward<Args>(arguments)...);
}

} // namespace monitoring
} // namespace dbus
} // namespace phosphor
