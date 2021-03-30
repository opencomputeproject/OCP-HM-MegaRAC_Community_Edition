/**
 * Copyright © 2016 IBM Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include "manager.hpp"
#include "xyz/openbmc_project/Example/Iface1/server.hpp"
#include "xyz/openbmc_project/Example/Iface2/server.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;

using Object = phosphor::inventory::manager::Object;
using ObjectMap = std::map<sdbusplus::message::object_path, Object>;

constexpr auto MGR_SERVICE = "phosphor.inventory.test.mgr";
constexpr auto MGR_INTERFACE = IFACE;
constexpr auto MGR_ROOT = "/testing/inventory";
constexpr auto EXAMPLE_SERVICE = "phosphor.inventory.test.example";
constexpr auto EXAMPLE_ROOT = "/testing";

const auto trigger1 =
    sdbusplus::message::object_path(EXAMPLE_ROOT + "/trigger1"s);
const auto trigger2 =
    sdbusplus::message::object_path(EXAMPLE_ROOT + "/trigger2"s);
const auto trigger3 =
    sdbusplus::message::object_path(EXAMPLE_ROOT + "/trigger3"s);
const auto trigger4 =
    sdbusplus::message::object_path(EXAMPLE_ROOT + "/trigger4"s);
const auto trigger5 =
    sdbusplus::message::object_path(EXAMPLE_ROOT + "/trigger5"s);

const sdbusplus::message::object_path relDeleteMeOne{"/deleteme1"};
const sdbusplus::message::object_path relDeleteMeTwo{"/deleteme2"};
const sdbusplus::message::object_path relDeleteMeThree{"/deleteme3"};

const std::string root{MGR_ROOT};
const std::string deleteMeOne{root + relDeleteMeOne.str};
const std::string deleteMeTwo{root + relDeleteMeTwo.str};
const std::string deleteMeThree{root + relDeleteMeThree.str};

using ExampleIface1 = sdbusplus::xyz::openbmc_project::Example::server::Iface1;
using ExampleIface2 = sdbusplus::xyz::openbmc_project::Example::server::Iface2;

/** @class ExampleService
 *  @brief Host an object for triggering events.
 */
struct ExampleService
{
    ~ExampleService() = default;
    ExampleService() :
        shutdown(false), bus(sdbusplus::bus::new_default()),
        objmgr(sdbusplus::server::manager::manager(bus, MGR_ROOT))
    {
        bus.request_name(EXAMPLE_SERVICE);
    }

    void run()
    {
        sdbusplus::server::object::object<ExampleIface1, ExampleIface2> t1(
            bus, trigger1.str.c_str());
        sdbusplus::server::object::object<ExampleIface1, ExampleIface2> t2(
            bus, trigger2.str.c_str());
        sdbusplus::server::object::object<ExampleIface1, ExampleIface2> t3(
            bus, trigger3.str.c_str());
        sdbusplus::server::object::object<ExampleIface1, ExampleIface2> t4(
            bus, trigger4.str.c_str());
        sdbusplus::server::object::object<ExampleIface1, ExampleIface2> t5(
            bus, trigger5.str.c_str());

        while (!shutdown)
        {
            bus.process_discard();
            bus.wait((5000000us).count());
        }
    }

    volatile bool shutdown;
    sdbusplus::bus::bus bus;
    sdbusplus::server::manager::manager objmgr;
};

/** @class SignalQueue
 *  @brief Store DBus signals in a queue.
 */
class SignalQueue
{
  public:
    ~SignalQueue() = default;
    SignalQueue() = delete;
    SignalQueue(const SignalQueue&) = delete;
    SignalQueue(SignalQueue&&) = default;
    SignalQueue& operator=(const SignalQueue&) = delete;
    SignalQueue& operator=(SignalQueue&&) = default;
    explicit SignalQueue(const std::string& match) :
        _bus(sdbusplus::bus::new_default()),
        _match(_bus, match.c_str(), &callback, this), _next(nullptr)
    {
    }

    auto&& pop(unsigned timeout = 1000000)
    {
        while (timeout > 0 && !_next)
        {
            _bus.process_discard();
            _bus.wait(50000);
            timeout -= 50000;
        }
        return std::move(_next);
    }

  private:
    static int callback(sd_bus_message* m, void* context, sd_bus_error*)
    {
        auto* me = static_cast<SignalQueue*>(context);
        sd_bus_message_ref(m);
        sdbusplus::message::message msg{m};
        me->_next = std::move(msg);
        return 0;
    }

    sdbusplus::bus::bus _bus;
    sdbusplus::bus::match_t _match;
    sdbusplus::message::message _next;
};

/**@brief Find a subset of interfaces and properties in an object. */
auto hasProperties(const Object& l, const Object& r)
{
    Object result;
    std::set_difference(r.cbegin(), r.cend(), l.cbegin(), l.cend(),
                        std::inserter(result, result.end()));
    return result.empty();
}

/**@brief Check an object for one or more interfaces. */
auto hasInterfaces(const std::vector<std::string>& l, const Object& r)
{
    std::vector<std::string> stripped, interfaces;
    std::transform(r.cbegin(), r.cend(), std::back_inserter(stripped),
                   [](auto& p) { return p.first; });
    std::set_difference(stripped.cbegin(), stripped.cend(), l.cbegin(),
                        l.cend(), std::back_inserter(interfaces));
    return interfaces.empty();
}

void runTests()
{
    const std::string exampleRoot{EXAMPLE_ROOT};
    auto b = sdbusplus::bus::new_default();

    auto notify = [&]() {
        return b.new_method_call(MGR_SERVICE, MGR_ROOT, MGR_INTERFACE,
                                 "Notify");
    };
    auto set = [&](const std::string& path) {
        return b.new_method_call(EXAMPLE_SERVICE, path.c_str(),
                                 "org.freedesktop.DBus.Properties", "Set");
    };

    Object obj{
        {"xyz.openbmc_project.Example.Iface1",
         {{"ExampleProperty1", "test1"s}}},
        {"xyz.openbmc_project.Example.Iface2",
         {{"ExampleProperty2", "test2"s},
          {"ExampleProperty3", static_cast<int64_t>(0ll)}}},
    };

    // Validate startup events occurred.
    {
        sdbusplus::message::object_path relCreateMe3{"/createme3"};
        std::string createMe3{root + relCreateMe3.str};

        auto get =
            b.new_method_call(MGR_SERVICE, createMe3.c_str(),
                              "org.freedesktop.DBus.Properties", "GetAll");
        get.append("xyz.openbmc_project.Example.Iface1");
        auto resp = b.call(get);

        Object::mapped_type properties;
        assert(!resp.is_method_error());
        resp.read(properties);
    }

    // Make sure the notify method works.
    {
        sdbusplus::message::object_path relPath{"/foo"};
        std::string path(root + relPath.str);

        SignalQueue queue("path='" + root + "',member='InterfacesAdded'");

        auto m = notify();
        m.append(ObjectMap({{relPath, obj}}));
        b.call(m);

        auto sig{queue.pop()};
        assert(static_cast<bool>(sig));
        sdbusplus::message::object_path signalPath;
        Object signalObjectType;
        sig.read(signalPath);
        assert(path == signalPath.str);
        sig.read(signalObjectType);
        assert(hasProperties(signalObjectType, obj));
        auto moreSignals{queue.pop()};
        assert(!moreSignals);
    }

    // Validate the propertyIs filter.
    {// Create an object to be deleted.
     {auto m = notify();
    m.append(ObjectMap({{relDeleteMeThree, obj}}));
    b.call(m);
}

// Validate that the action does not run if the property doesn't match.
{
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");
    auto m = set(trigger4.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty2");
    m.append(std::variant<std::string>("123"));
    b.call(m);
    auto sig{queue.pop()};
    assert(!sig);
}

// Validate that the action does run if the property matches.
{
    // Set ExampleProperty2 to something else to the 123 filter
    // matches.
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");
    auto m = set(trigger4.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty2");
    m.append(std::variant<std::string>("xyz"));
    b.call(m);
    auto sig{queue.pop()};
    assert(!sig);
}
{
    // Set ExampleProperty3 to 99.
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");
    auto m = set(trigger4.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty3");
    m.append(std::variant<int64_t>(99));
    b.call(m);
    auto sig{queue.pop()};
    assert(!sig);
}
{
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");
    auto m = set(trigger4.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty2");
    m.append(std::variant<std::string>("123"));
    b.call(m);

    sdbusplus::message::object_path sigpath;
    std::vector<std::string> interfaces;
    {
        std::vector<std::string> interfaces;
        auto sig{queue.pop()};
        assert(static_cast<bool>(sig));
        sig.read(sigpath);
        assert(sigpath == deleteMeThree);
        sig.read(interfaces);
        std::sort(interfaces.begin(), interfaces.end());
        assert(hasInterfaces(interfaces, obj));
    }
}
}

// Make sure DBus signals are handled.
{// Create some objects to be deleted by an action.
 {auto m = notify();
m.append(ObjectMap({{relDeleteMeOne, obj}}));
b.call(m);
}
{
    auto m = notify();
    m.append(ObjectMap({{relDeleteMeTwo, obj}}));
    b.call(m);
}
{
    auto m = notify();
    m.append(ObjectMap({{relDeleteMeThree, obj}}));
    b.call(m);
}

// Set some properties that should not trigger due to a filter.
{
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");
    auto m = set(trigger1.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty2");
    m.append(std::variant<std::string>("abc123"));
    b.call(m);
    auto sig{queue.pop()};
    assert(!sig);
}
{
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");
    auto m = set(trigger3.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty3");
    m.append(std::variant<int64_t>(11));
    b.call(m);
    auto sig{queue.pop()};
    assert(!sig);
}

// Set some properties that should trigger.
{
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");

    auto m = set(trigger1.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty2");
    m.append(std::variant<std::string>("xxxyyy"));
    b.call(m);

    sdbusplus::message::object_path sigpath;
    std::vector<std::string> interfaces;
    {
        std::vector<std::string> interfaces;
        auto sig{queue.pop()};
        assert(static_cast<bool>(sig));
        sig.read(sigpath);
        assert(sigpath == deleteMeOne);
        sig.read(interfaces);
        std::sort(interfaces.begin(), interfaces.end());
        assert(hasInterfaces(interfaces, obj));
    }
    {
        std::vector<std::string> interfaces;
        auto sig{queue.pop()};
        assert(static_cast<bool>(sig));
        sig.read(sigpath);
        assert(sigpath == deleteMeTwo);
        sig.read(interfaces);
        std::sort(interfaces.begin(), interfaces.end());
        assert(hasInterfaces(interfaces, obj));
    }
    {
        // Make sure there were only two signals.
        auto sig{queue.pop()};
        assert(!sig);
    }
}
{
    SignalQueue queue("path='" + root + "',member='InterfacesRemoved'");

    auto m = set(trigger3.str);
    m.append("xyz.openbmc_project.Example.Iface2");
    m.append("ExampleProperty3");
    m.append(std::variant<int64_t>(10));
    b.call(m);

    sdbusplus::message::object_path sigpath;
    std::vector<std::string> interfaces;
    {
        std::vector<std::string> interfaces;
        auto sig{queue.pop()};
        assert(static_cast<bool>(sig));
        sig.read(sigpath);
        assert(sigpath == deleteMeThree);
        sig.read(interfaces);
        std::sort(interfaces.begin(), interfaces.end());
        assert(hasInterfaces(interfaces, obj));
    }
    {
        // Make sure there was only one signal.
        auto sig{queue.pop()};
        assert(!sig);
    }
}
}

// Validate the set property action.
{
    sdbusplus::message::object_path relChangeMe{"/changeme"};
    std::string changeMe{root + relChangeMe.str};

    // Create an object to be updated by the set property action.
    {
        auto m = notify();
        m.append(ObjectMap({{relChangeMe, obj}}));
        b.call(m);
    }

    // Trigger and validate the change.
    {
        SignalQueue queue("path='" + changeMe + "',member='PropertiesChanged'");
        auto m = set(trigger2.str);
        m.append("xyz.openbmc_project.Example.Iface2");
        m.append("ExampleProperty2");
        m.append(std::variant<std::string>("yyyxxx"));
        b.call(m);

        std::string sigInterface;
        std::map<std::string, std::variant<std::string>> sigProperties;
        {
            std::vector<std::string> interfaces;
            auto sig{queue.pop()};
            sig.read(sigInterface);
            assert(sigInterface == "xyz.openbmc_project.Example.Iface1");
            sig.read(sigProperties);
            assert(std::get<std::string>(sigProperties["ExampleProperty1"]) ==
                   "changed");
        }
    }
}

// Validate the create object action.
{
    sdbusplus::message::object_path relCreateMe1{"/createme1"};
    sdbusplus::message::object_path relCreateMe2{"/createme2"};
    std::string createMe1{root + relCreateMe1.str};
    std::string createMe2{root + relCreateMe2.str};

    // Trigger the action.
    {
        sdbusplus::message::object_path signalPath;
        Object signalObject;

        SignalQueue queue("path='" + root + "',member='InterfacesAdded'");

        auto m = set(trigger5.str);
        m.append("xyz.openbmc_project.Example.Iface2");
        m.append("ExampleProperty2");
        m.append(std::variant<std::string>("abc123"));
        b.call(m);
        {
            auto sig{queue.pop()};
            assert(static_cast<bool>(sig));
            sig.read(signalPath);
            assert(createMe1 == signalPath.str);
            sig.read(signalObject);
        }
        {
            auto sig{queue.pop()};
            assert(static_cast<bool>(sig));
            sig.read(signalPath);
            assert(createMe2 == signalPath.str);
            sig.read(signalObject);
        }

        auto moreSignals{queue.pop()};
        assert(!moreSignals);
    }
}
}

int main()
{
    phosphor::inventory::manager::Manager mgr(
        sdbusplus::bus::new_default(), MGR_SERVICE, MGR_ROOT, MGR_INTERFACE);
    ExampleService d;

    auto f1 = [](auto mgr) { mgr->run(); };
    auto f2 = [](auto d) { d->run(); };

    auto t1 = std::thread(f1, &mgr);
    auto t2 = std::thread(f2, &d);

    runTests();

    mgr.shutdown();
    d.shutdown = true;

    // Wait for server threads to exit.
    t1.join();
    t2.join();
    std::cout << "Success!  Waiting for threads to exit..." << std::endl;

    return 0;
}

// vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
