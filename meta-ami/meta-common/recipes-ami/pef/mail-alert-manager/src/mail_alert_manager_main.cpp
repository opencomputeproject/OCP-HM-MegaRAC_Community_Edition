/********************************
 *SMTP Client Process
 *Author: selvaganapathim
 *Email : selvaganapathim@ami.com
 *
 * ******************************/
#include "mail_alert_manager.hpp"

#include <boost/asio/io_service.hpp>
#include <iostream>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message.hpp>

static constexpr const char* smtpclient = "xyz.openbmc_project.mail";
static constexpr const char* smtpObj = "/xyz/openbmc_project/mail/alert";
static constexpr const char* smtpIntf = "xyz.openbmc_project.mail.alert";

int main()
{
    boost::asio::io_service io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name(smtpclient);
    auto server = sdbusplus::asio::object_server(conn);
    std::shared_ptr<sdbusplus::asio::dbus_interface> smtpIface =
        server.add_interface(smtpObj, smtpIntf);
    mail::alert::manager::Smtp objsmtp;

    objsmtp.initializeSmtpcfg();

    // Register SendMail method
    smtpIface->register_method("SendMail", [&](const std::string& recipient,
                                               const std::string& subject,
                                               const std::string& msg) {
        return objsmtp.sendmail(recipient, subject, msg);
    });

    // Register getsmtpconfig method
    smtpIface->register_method("GetSmtpConfig",
                               [&]() { return objsmtp.getsmtpconfig(); });

    // Register setsmtpconfig method
    smtpIface->register_method(
        "SetSmtpConfig", [&](bool enable, const std::string& host,
                             const uint16_t& port, const std::string& sender) {
            return objsmtp.setsmtpconfig(enable, host, port, sender);
        });

    smtpIface->initialize();

    io.run();
    return 0;
}
