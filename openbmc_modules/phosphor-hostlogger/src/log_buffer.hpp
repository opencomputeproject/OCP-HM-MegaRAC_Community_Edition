// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#pragma once

#include <ctime>
#include <functional>
#include <list>
#include <string>

/**
 * @class LogBuffer
 * @brief Container with automatic log message rotation.
 */
class LogBuffer
{
  public:
    /**
     * @struct Message
     * @brief Single log message.
     */
    struct Message
    {
        /** @brief Message creation time. */
        time_t timeStamp;
        /** @brief Text of the message. */
        std::string text;
    };

    using container_t = std::list<Message>;

    /**
     * @brief Constructor.
     *
     * @param[in] maxSize max number of messages that can be stored
     * @param[in] maxTime max age of messages that can be stored, in minutes
     */
    LogBuffer(size_t maxSize, size_t maxTime);

    /**
     * @brief Add raw data from host's console output.
     *
     * @param[in] data pointer to raw data buffer
     * @param[in] sz size of the buffer in bytes
     */
    void append(const char* data, size_t sz);

    /**
     * @brief Set handler called if buffer is full.
     *
     * @param[in] cb callback function
     */
    void setFullHandler(std::function<void()> cb);

    /** @brief Clear (reset) container. */
    void clear();
    /** @brief Check container for empty. */
    bool empty() const;
    /** @brief Get container's iterator. */
    container_t::const_iterator begin() const;
    /** @brief Get container's iterator. */
    container_t::const_iterator end() const;

  private:
    /** @brief Remove the oldest messages from container. */
    void shrink();

  private:
    /** @brief Log message list. */
    container_t messages;
    /** @brief Flag to indicate that the last message is incomplete. */
    bool lastComplete;
    /** @brief Max number of messages that can be stored. */
    size_t sizeLimit;
    /** @brief Max age of messages (in minutes) that can be stored. */
    size_t timeLimit;
    /** @brief Callback function called if buffer is full. */
    std::function<void()> fullHandler;
};
