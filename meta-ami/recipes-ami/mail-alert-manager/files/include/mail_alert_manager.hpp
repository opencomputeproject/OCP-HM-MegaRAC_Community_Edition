#pragma once

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdarg.h>

#include <vector>
#include <openssl/ssl.h>
#include <auth-client.h>
#include <libesmtp.h>

#include <nlohmann/json.hpp>

constexpr const char *configFilePath =
        "/usr/share/alert/smtp-config.json";
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
//	smtp_recipient_t recipient;
	auth_context_t authctx;
	const smtp_status_t *status;
	struct sigaction sa;
	std::string host;
	uint16_t port = 0;
	std::string sender;
	bool enable = false;
	std::string recipient;
/*	char *subject = NULL;
	int nocrlf = 0;
	int noauth = 0;
	int to_cc_bcc = 0;
	char *file;
	FILE *fp;
	int c;
*/	enum notify_flags notify = Notify_NOTSET;


        uint16_t sendmail(const std::string& subject, const std::string& msg);
	std::tuple<bool,std::string,uint16_t,std::string,std::string> getsmtpconfig();
	uint16_t setsmtpconfig(const bool enable, const std::string& host, const uint16_t& port,
                                    const std::string& sender, const std::string& recipient);
	int initializeSmtpcfg();
};

}
}
}
