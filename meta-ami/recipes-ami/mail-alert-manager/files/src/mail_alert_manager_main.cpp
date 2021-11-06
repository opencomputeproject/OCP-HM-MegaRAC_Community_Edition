/********************************
 *SMTP Client Process
 *Author: selvaganapathim
 *Email : selvaganapathim@ami.com
 *
 * ******************************/
#include <iostream>
#include <boost/asio/io_service.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message.hpp>
#include "mail_alert_manager.hpp"

static constexpr const char *smtpclient = "xyz.openbmc_project.mail";
static constexpr const char *smtpObj = "/xyz/openbmc_project/mail/alert";
static constexpr const char *smtpIntf = "xyz.openbmc_project.mail.alert";

int main()
{
	boost::asio::io_service io;
        auto conn = std::make_shared<sdbusplus::asio::connection>(io);
        conn->request_name(smtpclient);
        auto server = sdbusplus::asio::object_server(conn);
        std::shared_ptr<sdbusplus::asio::dbus_interface> smtpIface = server.add_interface(smtpObj, smtpIntf);
	mail::alert::manager::Smtp objsmtp;

	objsmtp.initializeSmtpcfg();

	//Register SendMail method
	smtpIface->register_method(
        "SendMail", [&](const std::string& subject, const std::string& msg) {
			return objsmtp.sendmail(subject, msg);	
		});

	//Register getsmtpconfig method
        smtpIface->register_method(
        "GetSmtpConfig", [&]() {
                        return objsmtp.getsmtpconfig();
                });

	//Register setsmtpconfig method
        smtpIface->register_method(
        "SetSmtpConfig", [&](bool enable, const std::string& host, const uint16_t& port,
				const std::string& sender,const std::string& recipient) {
                        return objsmtp.setsmtpconfig(enable, host, port, sender, recipient);
                });

	smtpIface->initialize();

	io.run();
	return 0;
}
