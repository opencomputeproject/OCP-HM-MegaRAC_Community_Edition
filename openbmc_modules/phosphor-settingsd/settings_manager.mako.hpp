## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.
// WARNING: Generated header. Do not edit!
<%
import re
from collections import defaultdict
objects = settingsDict.keys()
sdbusplus_namespaces = []
sdbusplus_includes = []
props = defaultdict(list)
validators = defaultdict(tuple)

def get_setting_sdbusplus_type(setting_intf):
    setting = "sdbusplus::" + setting_intf.replace('.', '::')
    i = setting.rfind('::')
    setting = setting[:i] + '::server::' + setting[i+2:]
    return setting

def get_setting_type(path):
    path = path[1:]
    path = path.replace('/', '::')
    return path
%>\
#pragma once

% for object in objects:
    % for item in settingsDict[object]:
<%
    include = item['Interface']
    include = include.replace('.', '/')
    include = include + "/server.hpp"
    sdbusplus_includes.append(include)
%>\
    % endfor
% endfor
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <fstream>
#include <utility>
#include <experimental/filesystem>
#include <regex>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include "config.h"
#include <xyz/openbmc_project/Common/error.hpp>
using namespace phosphor::logging;

% for i in set(sdbusplus_includes):
#include "${i}"
% endfor

% for object in objects:
    % for item in settingsDict[object]:
<%
    ns = get_setting_sdbusplus_type(item['Interface'])
    i = ns.rfind('::')
    ns = ns[:i]
    sdbusplus_namespaces.append(ns)
%>\
    % endfor
% endfor

namespace phosphor
{
namespace settings
{

namespace fs = std::experimental::filesystem;

namespace persistent
{

// A setting d-bus object /foo/bar/baz is persisted in the filesystem with the
// same path. This eases re-construction of settings objects when we restore
// from the filesystem. This can be a problem though when you have two objects
// such as - /foo/bar and /foo/bar/baz. This is because 'bar' will be treated as
// a file in the first case, and a subdir in the second. To solve this, suffix
// files with a trailing __. The __ is a safe character sequence to use, because
// we won't have d-bus object paths ending with this.
// With this the objects would be persisted as - /foo/bar__ and /foo/bar/baz__.
constexpr auto fileSuffix = "__";

}

% for n in set(sdbusplus_namespaces):
using namespace ${n};
% endfor

% for object in objects:
<%
   ns = object.split('/')
   ns.pop(0)
%>\
% for n in ns:
namespace ${n}
{
% endfor
<%
    interfaces = []
    aliases = []
    for item in settingsDict[object]:
        interfaces.append(item['Interface'])
        for name, meta in item['Properties'].items():
            if 'Validation' in meta:
                dict = meta['Validation']
                if dict['Type'] == "range":
                    validators[name] = (dict['Type'], dict['Validator'], dict['Unit'])
                else:
                    validators[name] = (dict['Type'], dict['Validator'])
%>
% for index, intf in enumerate(interfaces):
using Iface${index} = ${get_setting_sdbusplus_type(intf)};
<% aliases.append("Iface" + str(index)) %>\
% endfor
<%
    parent = "sdbusplus::server::object::object" + "<" + ", ".join(aliases) + ">"
%>\
using Parent = ${parent};

class Impl : public Parent
{
    public:
        Impl(sdbusplus::bus::bus& bus, const char* path):
            Parent(bus, path, true),
            path(path)
        {
        }
        virtual ~Impl() = default;

% for index, item in enumerate(settingsDict[object]):
    % for propName, metaDict in item['Properties'].items():
<% t = propName[:1].lower() + propName[1:] %>\
<% fname = "validate" + propName %>\
        decltype(std::declval<Iface${index}>().${t}()) ${t}(decltype(std::declval<Iface${index}>().${t}()) value) override
        {
            auto result = Iface${index}::${t}();
            if (value != result)
            {
            % if propName in validators:
                if (!${fname}(value))
                {
                    namespace error =
                        sdbusplus::xyz::openbmc_project::Common::Error;
                    namespace metadata =
                        phosphor::logging::xyz::openbmc_project::Common;
                    phosphor::logging::report<error::InvalidArgument>(
                        metadata::InvalidArgument::ARGUMENT_NAME("${t}"),
                    % if validators[propName][0] != "regex":
                        metadata::InvalidArgument::ARGUMENT_VALUE(std::to_string(value).c_str()));
                    % else:
                        metadata::InvalidArgument::ARGUMENT_VALUE(value.c_str()));
                    % endif
                    return result;
                }
             % endif
                fs::path p(SETTINGS_PERSIST_PATH);
                p /= path;
                p += persistent::fileSuffix;
                fs::create_directories(p.parent_path());
                std::ofstream os(p.c_str(), std::ios::binary);
                cereal::JSONOutputArchive oarchive(os);
                result = Iface${index}::${t}(value);
                oarchive(*this);
            }
            return result;
        }
        using Iface${index}::${t};

    % endfor
% endfor
    private:
        fs::path path;
% for index, item in enumerate(settingsDict[object]):
  % for propName, metaDict in item['Properties'].items():
<% t = propName[:1].lower() + propName[1:] %>\
<% fname = "validate" + propName %>\
    % if propName in validators:

        bool ${fname}(decltype(std::declval<Iface${index}>().${t}()) value)
        {
            bool matched = false;
        % if (validators[propName][0] == 'regex'):
            std::regex regexToCheck("${validators[propName][1]}");
            matched = std::regex_search(value, regexToCheck);
            if (!matched)
            {
                std::string err = "Input parameter for ${propName} is invalid "
                    "Input: " + value + " not in the format of this regex: "
                    "${validators[propName][1]}";
                using namespace phosphor::logging;
                log<level::ERR>(err.c_str());
            }
        % elif (validators[propName][0] == 'range'):
<% lowhigh = re.split('\.\.', validators[propName][1]) %>\
            if ((value <= ${lowhigh[1]}) && (value >= ${lowhigh[0]}))
            {
                matched = true;
            }
            else
            {
                std::string err = "Input parameter for ${propName} is invalid "
                    "Input: " + std::to_string(value) + "in uint: "
                    "${validators[propName][2]} is not in range:${validators[propName][1]}";
                using namespace phosphor::logging;
                log<level::ERR>(err.c_str());
            }
        % else:
            <% assert("Unknown validation type: propName") %>\
        % endif
            return matched;
        }
    % endif
  % endfor
% endfor
};

template<class Archive>
void save(Archive& a,
          const Impl& setting,
          const std::uint32_t version)
{
<%
props = []
for index, item in enumerate(settingsDict[object]):
    intfProps = ["setting." + propName[:1].lower() + propName[1:] + "()" for \
                    propName, metaDict in item['Properties'].items()]
    props.extend(intfProps)
props = ', '.join(props)
%>\
    a(${props});
}

template<class Archive>
void load(Archive& a,
          Impl& setting,
          const std::uint32_t version)
{
<% props = [] %>\
% for index, item in enumerate(settingsDict[object]):
  % for  prop, metaDict in item['Properties'].items():
<%
    t = "setting." + prop[:1].lower() + prop[1:] + "()"
    props.append(prop)
%>\
    decltype(${t}) ${prop}{};
  % endfor
% endfor
<% props = ', '.join(props) %>
    a(${props});
<% props = [] %>
% for index, item in enumerate(settingsDict[object]):
  % for  prop, metaDict in item['Properties'].items():
<%
    t = "setting." + prop[:1].lower() + prop[1:] + "(" + prop + ")"
%>\
    ${t};
  % endfor
% endfor
}

% for n in reversed(ns):
} // namespace ${n}
% endfor

% endfor

/** @class Manager
 *
 *  @brief Compose settings objects and put them on the bus.
 */
class Manager
{
    public:
        Manager() = delete;
        Manager(const Manager&) = delete;
        Manager& operator=(const Manager&) = delete;
        Manager(Manager&&) = delete;
        Manager& operator=(Manager&&) = delete;
        virtual ~Manager() = default;

        /** @brief Constructor to put settings objects on to the bus.
         *  @param[in] bus - Bus to attach to.
         */
        Manager(sdbusplus::bus::bus& bus)
        {
            fs::path path{};
            settings =
                std::make_tuple(
% for index, path in enumerate(objects):
<% type = get_setting_type(path) + "::Impl" %>\
                    std::make_unique<${type}>(
                        bus,
  % if index < len(settingsDict) - 1:
                        "${path}"),
  % else:
                        "${path}"));
  % endif
% endfor

% for index, path in enumerate(objects):
            path = fs::path(SETTINGS_PERSIST_PATH) / "${path}";
            path += persistent::fileSuffix;
            auto initSetting${index} = [&]()
            {
  % for item in settingsDict[path]:
    % for propName, metaDict in item['Properties'].items():
<% p = propName[:1].lower() + propName[1:] %>\
<% defaultValue = metaDict['Default'] %>\
                std::get<${index}>(settings)->
                  ${get_setting_sdbusplus_type(item['Interface'])}::${p}(${defaultValue});
  % endfor
% endfor
            };

            try
            {
                if (fs::exists(path))
                {
                    std::ifstream is(path.c_str(), std::ios::in);
                    cereal::JSONInputArchive iarchive(is);
                    iarchive(*std::get<${index}>(settings));
                }
                else
                {
                    initSetting${index}();
                }
            }
            catch (cereal::Exception& e)
            {
                log<level::ERR>(e.what());
                fs::remove(path);
                initSetting${index}();
            }
            std::get<${index}>(settings)->emit_object_added();

% endfor
        }

    private:
        /* @brief Composition of settings objects. */
        std::tuple<
% for index, path in enumerate(objects):
<% type = get_setting_type(path) + "::Impl" %>\
  % if index < len(settingsDict) - 1:
            std::unique_ptr<${type}>,
  % else:
            std::unique_ptr<${type}>> settings;
  % endif
% endfor
};

} // namespace settings
} // namespace phosphor

// Now register the class version with Cereal
% for object in objects:
<%
   classname = "phosphor::settings"
   ns = object.split('/')
   ns.pop(0)
%>\
% for n in ns:
<%
    classname += "::" + n
%>\
% endfor
CEREAL_CLASS_VERSION(${classname + "::Impl"}, CLASS_VERSION);
% endfor
