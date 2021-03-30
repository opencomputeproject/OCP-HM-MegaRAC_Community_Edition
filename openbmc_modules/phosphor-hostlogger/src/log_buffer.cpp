// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2020 YADRO

#include "log_buffer.hpp"

/** @brief Check if a character is EOL symbol. */
constexpr bool isEol(char c)
{
    return c == '\r' || c == '\n';
}

LogBuffer::LogBuffer(size_t maxSize, size_t maxTime) :
    lastComplete(true), sizeLimit(maxSize), timeLimit(maxTime)
{}

void LogBuffer::append(const char* data, size_t sz)
{
    // Split raw data into separate messages by EOL symbols (\r or \n).
    // Stream may not be ended with EOL, so we handle this situation by
    // lastComplete flag.
    size_t pos = 0;
    while (pos < sz)
    {
        // Search for EOL ('\r' or '\n')
        size_t eol = pos;
        while (eol < sz)
        {
            if (isEol(data[eol]))
            {
                break;
            }
            ++eol;
        }
        const bool eolFound = eol < sz;
        const char* msgText = data + pos;
        const size_t msgLen = (eolFound ? eol : sz) - pos;

        // Append message to the container
        if (!lastComplete && !messages.empty())
        {
            // The last message is incomplete, add data as part of it
            messages.back().text.append(msgText, msgLen);
        }
        else
        {
            Message msg;
            time(&msg.timeStamp);
            msg.text.assign(msgText, msgLen);
            messages.push_back(msg);
        }
        lastComplete = eolFound;

        // Move current position and skip EOL character
        pos = eol + 1;
        // Handle EOL sequences '\r\n' or '\n\r' as one delimiter
        if (eolFound && pos < sz && isEol(data[pos]) && data[eol] != data[pos])
        {
            ++pos;
        }
    }

    shrink();
}

void LogBuffer::setFullHandler(std::function<void()> cb)
{
    fullHandler = cb;
}

void LogBuffer::clear()
{
    messages.clear();
    lastComplete = true;
}

bool LogBuffer::empty() const
{
    return messages.empty();
}

LogBuffer::container_t::const_iterator LogBuffer::begin() const
{
    return messages.begin();
}

LogBuffer::container_t::const_iterator LogBuffer::end() const
{
    return messages.end();
}

void LogBuffer::shrink()
{
    if (sizeLimit && messages.size() > sizeLimit)
    {
        if (fullHandler)
        {
            fullHandler();
        }
        while (messages.size() > sizeLimit)
        {
            messages.pop_front();
        }
    }
    if (timeLimit && !messages.empty())
    {
        time_t oldest;
        time(&oldest);
        oldest -= timeLimit * 60 /* sec */;
        if (messages.begin()->timeStamp < oldest)
        {
            if (fullHandler)
            {
                fullHandler();
            }
            while (!messages.empty() && messages.begin()->timeStamp < oldest)
            {
                messages.pop_front();
            }
        }
    }
}
