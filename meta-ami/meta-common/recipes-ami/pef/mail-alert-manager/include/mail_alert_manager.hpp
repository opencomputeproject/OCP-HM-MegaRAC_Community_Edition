#pragma once

#define _XOPEN_SOURCE 500

#include <auth-client.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libesmtp.h>
#include <openssl/ssl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
#include <vector>

constexpr const char* configFilePath = "/var/lib/alert/smtp-config.json";

namespace mail
{
namespace alert
{
namespace manager
{
using Json = nlohmann::json;

class Smtp
{
  public:
    smtp_session_t session;
    smtp_message_t message;
    auth_context_t authctx;
    const smtp_status_t* status;
    struct sigaction sa;
    std::string host;
    uint16_t port = 0;
    std::string sender;
    bool enable = false;
    enum notify_flags notify = Notify_NOTSET;

    uint16_t sendmail(const std::string& recipient, const std::string& subject,
                      const std::string& msg);
    std::tuple<bool, std::string, uint16_t, std::string> getsmtpconfig();
    uint16_t setsmtpconfig(const bool enable, const std::string& host,
                           const uint16_t& port, const std::string& sender);
    int initializeSmtpcfg();
};

} // namespace manager
} // namespace alert
} // namespace mail
