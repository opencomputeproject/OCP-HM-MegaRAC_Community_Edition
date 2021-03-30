// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "dbus_loop.hpp"

#include <phosphor-logging/log.hpp>

#include <system_error>

using namespace phosphor::logging;

DbusLoop::DbusLoop() : bus(nullptr), event(nullptr)
{
    int rc;

    rc = sd_bus_default(&bus);
    if (rc < 0)
    {
        std::error_code ec(-rc, std::generic_category());
        throw std::system_error(ec, "Unable to initiate D-Bus connection");
    }

    rc = sd_event_default(&event);
    if (rc < 0)
    {
        sd_bus_unref(bus);
        std::error_code ec(-rc, std::generic_category());
        throw std::system_error(ec, "Unable to create D-Bus event loop");
    }

    rc = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
    if (rc < 0)
    {
        sd_bus_unref(bus);
        sd_event_unref(event);
        std::error_code ec(-rc, std::generic_category());
        throw std::system_error(ec, "Unable to attach D-Bus event");
    }
}

DbusLoop::~DbusLoop()
{
    sd_bus_unref(bus);
    sd_event_unref(event);
}

int DbusLoop::run() const
{
    return sd_event_loop(event);
}

void DbusLoop::stop(int code) const
{
    sd_event_exit(event, code);
}

void DbusLoop::addPropertyHandler(const std::string& objPath,
                                  const WatchProperties& props,
                                  std::function<void()> callback)
{
    // Add match handler
    const int rc = sd_bus_match_signal(bus, nullptr, nullptr, objPath.c_str(),
                                       "org.freedesktop.DBus.Properties",
                                       "PropertiesChanged", msgCallback, this);
    if (rc < 0)
    {
        std::error_code ec(-rc, std::generic_category());
        throw std::system_error(ec, "Unable to register property watcher");
    }

    propWatch = props;
    propHandler = callback;
}

void DbusLoop::addIoHandler(int fd, std::function<void()> callback)
{
    ioHandler = callback;
    const int rc = sd_event_add_io(event, nullptr, fd, EPOLLIN,
                                   &DbusLoop::ioCallback, this);
    if (rc < 0)
    {
        std::error_code ec(-rc, std::generic_category());
        throw std::system_error(ec, "Unable to register IO handler");
    }
}

void DbusLoop::addSignalHandler(int signal, std::function<void()> callback)
{
    // Block the signal
    sigset_t ss;
    if (sigemptyset(&ss) < 0 || sigaddset(&ss, signal) < 0 ||
        sigprocmask(SIG_BLOCK, &ss, nullptr) < 0)
    {
        std::error_code ec(errno, std::generic_category());
        std::string err = "Unable to block signal ";
        err += strsignal(signal);
        throw std::system_error(ec, err);
    }

    signalHandlers.insert(std::make_pair(signal, callback));

    // Register handler
    const int rc = sd_event_add_signal(event, nullptr, signal,
                                       &DbusLoop::signalCallback, this);
    if (rc < 0)
    {
        std::error_code ec(-rc, std::generic_category());
        std::string err = "Unable to register handler for signal ";
        err += strsignal(signal);
        throw std::system_error(ec, err);
    }
}

int DbusLoop::msgCallback(sd_bus_message* msg, void* userdata,
                          sd_bus_error* /*err*/)
{
    const WatchProperties& propWatch =
        static_cast<DbusLoop*>(userdata)->propWatch;

    try
    {
        int rc;

        // Filter out by interface name
        const char* interface;
        rc = sd_bus_message_read(msg, "s", &interface);
        if (rc < 0)
        {
            std::error_code ec(-rc, std::generic_category());
            throw std::system_error(ec, "Unable to read interface name");
        }
        const auto& itIface = propWatch.find(interface);
        if (itIface == propWatch.end())
        {
            return 0; // Interface is now watched
        }
        const Properties& props = itIface->second;

        // Read message: go through list of changed properties
        rc = sd_bus_message_enter_container(msg, SD_BUS_TYPE_ARRAY, "{sv}");
        if (rc < 0)
        {
            std::error_code ec(-rc, std::generic_category());
            throw std::system_error(ec, "Unable to open message container");
        }
        while ((rc = sd_bus_message_enter_container(msg, SD_BUS_TYPE_DICT_ENTRY,
                                                    "sv")) > 0)
        {
            // Get property's name
            const char* name;
            rc = sd_bus_message_read(msg, "s", &name);
            if (rc < 0)
            {
                sd_bus_message_exit_container(msg);
                std::error_code ec(-rc, std::generic_category());
                throw std::system_error(ec, "Unable to get property name");
            }

            // Get and check property's type
            const char* type;
            rc = sd_bus_message_peek_type(msg, nullptr, &type);
            if (rc < 0 || strcmp(type, "s"))
            {
                sd_bus_message_exit_container(msg);
                continue;
            }

            // Get property's value
            const char* value;
            rc = sd_bus_message_enter_container(msg, SD_BUS_TYPE_VARIANT, type);
            if (rc < 0)
            {
                sd_bus_message_exit_container(msg);
                std::error_code ec(-rc, std::generic_category());
                throw std::system_error(ec, "Unable to open property value");
            }
            rc = sd_bus_message_read(msg, type, &value);
            if (rc < 0)
            {
                sd_bus_message_exit_container(msg);
                sd_bus_message_exit_container(msg);
                std::error_code ec(-rc, std::generic_category());
                throw std::system_error(ec, "Unable to get property value");
            }
            sd_bus_message_exit_container(msg);

            // Check property name/value and handle the match
            const auto& itProps = props.find(name);
            if (itProps != props.end() &&
                itProps->second.find(value) != itProps->second.end())
            {
                static_cast<DbusLoop*>(userdata)->propHandler();
            }

            sd_bus_message_exit_container(msg);
        }
        sd_bus_message_exit_container(msg);
    }
    catch (const std::exception& ex)
    {
        log<level::WARNING>(ex.what());
    }

    return 0;
}

int DbusLoop::signalCallback(sd_event_source* /*src*/,
                             const struct signalfd_siginfo* si, void* userdata)
{
    DbusLoop* instance = static_cast<DbusLoop*>(userdata);
    const auto it = instance->signalHandlers.find(si->ssi_signo);
    if (it != instance->signalHandlers.end())
    {
        it->second();
    }
    else
    {
        std::string msg = "Unhandled signal ";
        msg += strsignal(si->ssi_signo);
        log<level::WARNING>(msg.c_str());
    }
    return 0;
}

int DbusLoop::ioCallback(sd_event_source* /*src*/, int /*fd*/,
                         uint32_t /*revents*/, void* userdata)
{
    static_cast<DbusLoop*>(userdata)->ioHandler();
    return 0;
}
