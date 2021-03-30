## This file is a template, the comment below is emitted into the generated file
/* This is an auto generated file. Do not edit. */
#pragma once

#include <array>
#include <chrono>
#include <string>
#include "count.hpp"
#include "median.hpp"
#include "data_types.hpp"
#include "journal.hpp"
#include "elog.hpp"
#include "errors.hpp"
#include "method.hpp"
#include "propertywatchimpl.hpp"
#include "pathwatchimpl.hpp"
#include "resolve_errors.hpp"
#include "sdbusplus.hpp"
#include "event.hpp"
#include "snmp_trap.hpp"

using namespace std::string_literals;
using namespace std::chrono_literals;

namespace phosphor
{
namespace dbus
{
namespace monitoring
{

struct ConfigMeta
{
    using Meta = std::array<std::string, ${len(meta)}>;

    static auto& get()
    {
        static const Meta meta =
        {
% for m in meta:
            "${m.name}"s,
% endfor
        };
        return meta;
    }
};

struct ConfigPaths
{
    using Paths = std::array<std::string, ${len(paths)}>;

    static auto& get()
    {
        static const Paths paths =
        {
% for p in paths:
            "${p.name}"s,
% endfor
        };
        return paths;
    }
};

struct ConfigInterfaces
{
    using Interfaces = std::array<std::string, ${len(interfaces)}>;

    static auto& get()
    {
        static const Interfaces interfaces =
        {
% for i in interfaces:
            "${i.name}"s,
% endfor
        };
        return interfaces;
    }
};

struct ConfigIntfAddPaths
{
    using Paths = std::array<std::string, ${len(pathinstances)}>;

    static auto& get()
    {
        static const Paths paths =
        {
% for p in pathinstances:
            "${p.path}"s,
% endfor
        };
        return paths;
    }
};

struct ConfigProperties
{
    using Properties = std::array<std::string, ${len(propertynames)}>;

    static auto& get()
    {
        static const Properties properties =
        {
% for p in propertynames:
            "${p.name}"s,
% endfor
        };
        return properties;
    }
};

struct ConfigPropertyStorage
{
    using Storage = std::array<std::tuple<any_ns::any, any_ns::any>, ${len(instances)}>;

    static auto& get()
    {
        static Storage storage;
        return storage;
    }
};

struct ConfigPropertyFilters
{
    using PropertyFilters = std::array<std::unique_ptr<Filters>, ${len(filters)}>;

    static auto& get()
    {
        static const PropertyFilters propertyFilters =
        {
% for f in filters:
            std::make_unique<OperandFilters<${f.datatype}>>(
                std::vector<std::function<bool(${f.datatype})>>{
    % for o in f.filters:
                    [](const auto& val){
                        return val ${o.op} ${o.argument(loader, indent=indent +1)};
                    },
    % endfor
                }
            ),
% endfor
        };
        return propertyFilters;
    }
};

struct ConfigPropertyIndicies
{
    using PropertyIndicies = std::array<PropertyIndex, ${len(instancegroups)}>;

    static auto& get()
    {
        static const PropertyIndicies propertyIndicies =
        {
            {
% for g in instancegroups:
                {
    % for i in g.members:
                    {
                        PropertyIndex::key_type
                        {
                            ConfigPaths::get()[${i[0]}],
                            ConfigInterfaces::get()[${i[2]}],
                            ConfigProperties::get()[${i[3]}]
                        },
                        PropertyIndex::mapped_type
                        {
                            ConfigMeta::get()[${i[1]}],
                            ConfigMeta::get()[${i[4]}],
                            ConfigPropertyStorage::get()[${i[5]}]
                        },
                    },
    % endfor
                },
% endfor
            }
        };
        return propertyIndicies;
    }
};

struct ConfigPropertyCallbackGroups
{
    using CallbackGroups = std::array<std::vector<size_t>, ${len(callbackgroups)}>;
    static auto& get()
    {
        static const CallbackGroups propertyCallbackGraph =
        {
            {
% for g in callbackgroups:
                {${', '.join([str(x) for x in g.members])}},
% endfor
            }
        };
        return propertyCallbackGraph;
    }
};

struct ConfigConditions
{
    using Conditions = std::array<std::unique_ptr<Conditional>, ${len(conditions)}>;

    static auto& get()
    {
        static const Conditions propertyConditions =
        {
% for c in conditions:
            ${c.construct(loader, indent=indent +3)},
% endfor
        };
        return propertyConditions;
    }
};

struct ConfigPropertyCallbacks
{
    using Callbacks = std::array<std::unique_ptr<Callback>, ${len(callbacks)}>;

    static auto& get()
    {
        static const Callbacks propertyCallbacks =
        {
% for c in callbacks:
            ${c.construct(loader, indent=indent +3)},
% endfor
        };
        return propertyCallbacks;
    }
};

struct ConfigPathCallbacks
{
    using Callbacks = std::array<std::unique_ptr<Callback>, ${len(pathcallbacks)}>;

    static auto& get()
    {
        static const Callbacks pathCallbacks =
        {
% for c in pathcallbacks:
            ${c.construct(loader, indent=indent +3)},
% endfor
        };
        return pathCallbacks;
    }
};

struct ConfigPropertyWatches
{
    using PropertyWatches = std::array<std::unique_ptr<Watch>, ${len(watches)}>;

    static auto& get()
    {
        static const PropertyWatches propertyWatches =
        {
% for w in watches:
            std::make_unique<PropertyWatchOfType<${w.datatype}, SDBusPlus>>(
    % if w.callback is None:
        % if w.filters is None:
                ConfigPropertyIndicies::get()[${w.instances}]),
        % else:
                ConfigPropertyIndicies::get()[${w.instances}],
                ConfigPropertyFilters::get()[${w.filters}].get()),
        % endif
    % else:
        % if w.filters is None:
                ConfigPropertyIndicies::get()[${w.instances}],
                *ConfigPropertyCallbacks::get()[${w.callback}]),
        % else:
                ConfigPropertyIndicies::get()[${w.instances}],
                *ConfigPropertyCallbacks::get()[${w.callback}],
                ConfigPropertyFilters::get()[${w.filters}].get()),
        % endif
    % endif
% endfor
        };
        return propertyWatches;
    }
};

struct ConfigPathWatches
{
    using PathWatches = std::array<std::unique_ptr<Watch>, ${len(pathwatches)}>;

    static auto& get()
    {
        static const PathWatches pathWatches =
        {
% for w in pathwatches:
            std::make_unique<PathWatch<SDBusPlus>>(
    % if w.pathcallback is None:
                ConfigIntfAddPaths::get()[${w.pathinstances}]),
    % else:
                ConfigIntfAddPaths::get()[${w.pathinstances}],
                *ConfigPathCallbacks::get()[${w.pathcallback}]),
    % endif
% endfor
        };
        return pathWatches;
    }
};
} // namespace monitoring
} // namespace dbus
} // namespace phosphor
