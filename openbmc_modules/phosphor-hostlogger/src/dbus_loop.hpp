// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <systemd/sd-bus.h>

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

/**
 * @class DbusLoop
 * @brief D-Bus based event loop.
 */
class DbusLoop
{
  public:
    /** @brief Set of possible values of the property. */
    using PropertyValues = std::set<std::string>;
    /** @brief Map of properties: name -> watched values. */
    using Properties = std::map<std::string, PropertyValues>;
    /** @brief Map of watched properties: interface -> properties. */
    using WatchProperties = std::map<std::string, Properties>;

    DbusLoop();
    ~DbusLoop();

    /**
     * @brief Run worker loop.
     *
     * @return exit code from loop
     */
    int run() const;

    /**
     * @brief Stop worker loop.
     *
     * @param[in] code exit code
     */
    void stop(int code) const;

    /**
     * @brief Add property change handler.
     *
     * @param[in] service D-Bus service name (object owner)
     * @param[in] objPath path to the D-Bus object
     * @param[in] props watched properties description
     * @param[in] callback function to call when property get one the listed
     *            values
     *
     * @throw std::system_error in case of errors
     */
    void addPropertyHandler(const std::string& objPath,
                            const WatchProperties& props,
                            std::function<void()> callback);

    /**
     * @brief Add IO event handler.
     *
     * @param[in] fd file descriptor to watch
     * @param[in] callback function to call when IO event is occurred
     *
     * @throw std::system_error in case of errors
     */
    void addIoHandler(int fd, std::function<void()> callback);

    /**
     * @brief Add signal handler.
     *
     * @param[in] signal signal to watch
     * @param[in] callback function to call when signal is triggered
     *
     * @throw std::system_error in case of errors
     */
    void addSignalHandler(int signal, std::function<void()> callback);

  private:
    /**
     * @brief D-Bus callback: message handler.
     *        See sd_bus_message_handler_t for details.
     */
    static int msgCallback(sd_bus_message* msg, void* userdata,
                           sd_bus_error* err);

    /**
     * @brief D-Bus callback: signal handler.
     *        See sd_event_signal_handler_t for details.
     */
    static int signalCallback(sd_event_source* src,
                              const struct signalfd_siginfo* si,
                              void* userdata);

    /**
     * @brief D-Bus callback: IO handler.
     *        See sd_event_io_handler_t for details.
     */
    static int ioCallback(sd_event_source* src, int fd, uint32_t revents,
                          void* userdata);

  private:
    /** @brief D-Bus connection. */
    sd_bus* bus;
    /** @brief D-Bus event loop. */
    sd_event* event;

    /** @brief Watched properties. */
    WatchProperties propWatch;
    /** @brief Property change handler. */
    std::function<void()> propHandler;

    /** @brief IO handler. */
    std::function<void()> ioHandler;

    /** @brief Signal handlers. */
    std::map<int, std::function<void()>> signalHandlers;
};
