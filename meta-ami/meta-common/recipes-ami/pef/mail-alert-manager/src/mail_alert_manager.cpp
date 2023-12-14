#include "mail_alert_manager.hpp"

#include <syslog.h>

#include <fstream>
#include <iostream>

namespace mail
{
namespace alert
{
namespace manager
{
void monitor_cb(const char* buf, int buflen, int writing, void* arg)
{
    if (writing == SMTP_CB_HEADERS)
    {
        std::cerr << "H: ";
        std::cerr << buf;
        return;
    }

    if (writing)
        std::cerr << "C: ";
    else
        std::cerr << "S: ";

    std::cerr << buf;

    if (buf[buflen - 1] != '\n')
        std::cerr << "\n";
}

/* Callback to request user/password info.  Not thread safe. */
int authinteract(auth_client_request_t request, char** result, int fields,
                 void* arg)
{
    char prompt[64];
    static char resp[512];
    char *p, *rp;
    int i, n, tty;

    rp = resp;
    for (i = 0; i < fields; i++)
    {
        n = snprintf(prompt, sizeof prompt, "%s%s: ", request[i].prompt,
                     (request[i].flags & AUTH_CLEARTEXT) ? " (not encrypted)"
                                                         : "");
        if (request[i].flags & AUTH_PASS)
            result[i] = getpass(prompt);
        else
        {
            tty = open("/dev/tty", O_RDWR);
            write(tty, prompt, n);
            n = read(tty, rp, sizeof resp - (rp - resp));
            close(tty);
            p = rp + n;
            while (isspace(p[-1]))
                p--;
            *p++ = '\0';
            result[i] = rp;
            rp = p;
        }
    }
    return 1;
}

int tlsinteract(char* buf, int buflen, int rwflag, void* arg)
{
    char* pw;
    int len;

    pw = getpass("certificate password");
    len = strlen(pw);
    if (len + 1 > buflen)
        return 0;
    strcpy(buf, pw);
    return len;
}

int handle_invalid_peer_certificate(long vfy_result)
{
    const char* k = "rare error";
    switch (vfy_result)
    {
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            k = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT";
            break;
        case X509_V_ERR_UNABLE_TO_GET_CRL:
            k = "X509_V_ERR_UNABLE_TO_GET_CRL";
            break;
        case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
            k = "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE";
            break;
        case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
            k = "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE";
            break;
        case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
            k = "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY";
            break;
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
            k = "X509_V_ERR_CERT_SIGNATURE_FAILURE";
            break;
        case X509_V_ERR_CRL_SIGNATURE_FAILURE:
            k = "X509_V_ERR_CRL_SIGNATURE_FAILURE";
            break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
            k = "X509_V_ERR_CERT_NOT_YET_VALID";
            break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
            k = "X509_V_ERR_CERT_HAS_EXPIRED";
            break;
        case X509_V_ERR_CRL_NOT_YET_VALID:
            k = "X509_V_ERR_CRL_NOT_YET_VALID";
            break;
        case X509_V_ERR_CRL_HAS_EXPIRED:
            k = "X509_V_ERR_CRL_HAS_EXPIRED";
            break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
            k = "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD";
            break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
            k = "X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD";
            break;
        case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
            k = "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD";
            break;
        case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
            k = "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD";
            break;
        case X509_V_ERR_OUT_OF_MEM:
            k = "X509_V_ERR_OUT_OF_MEM";
            break;
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
            k = "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT";
            break;
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            k = "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN";
            break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
            k = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY";
            break;
        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
            k = "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE";
            break;
        case X509_V_ERR_CERT_CHAIN_TOO_LONG:
            k = "X509_V_ERR_CERT_CHAIN_TOO_LONG";
            break;
        case X509_V_ERR_CERT_REVOKED:
            k = "X509_V_ERR_CERT_REVOKED";
            break;
        case X509_V_ERR_INVALID_CA:
            k = "X509_V_ERR_INVALID_CA";
            break;
        case X509_V_ERR_PATH_LENGTH_EXCEEDED:
            k = "X509_V_ERR_PATH_LENGTH_EXCEEDED";
            break;
        case X509_V_ERR_INVALID_PURPOSE:
            k = "X509_V_ERR_INVALID_PURPOSE";
            break;
        case X509_V_ERR_CERT_UNTRUSTED:
            k = "X509_V_ERR_CERT_UNTRUSTED";
            break;
        case X509_V_ERR_CERT_REJECTED:
            k = "X509_V_ERR_CERT_REJECTED";
            break;
    }
    std::cerr << "SMTP_EV_INVALID_PEER_CERTIFICATE: " << vfy_result << k
              << "\n";
    return 1; /* Accept the problem */
}

void event_cb(smtp_session_t session, int event_no, void* arg, ...)
{
    va_list alist;
    int* ok;

    va_start(alist, arg);
    switch (event_no)
    {
        case SMTP_EV_CONNECT:
        case SMTP_EV_MAILSTATUS:
        case SMTP_EV_RCPTSTATUS:
        case SMTP_EV_MESSAGEDATA:
        case SMTP_EV_MESSAGESENT:
        case SMTP_EV_DISCONNECT:
            break;
        case SMTP_EV_WEAK_CIPHER: {
            int bits;
            bits = va_arg(alist, long);
            ok = va_arg(alist, int*);
            printf("SMTP_EV_WEAK_CIPHER, bits=%d - accepted.\n", bits);
            *ok = 1;
            break;
        }
        case SMTP_EV_STARTTLS_OK:
            puts("SMTP_EV_STARTTLS_OK - TLS started here.");
            break;
        case SMTP_EV_INVALID_PEER_CERTIFICATE: {
            long vfy_result;
            vfy_result = va_arg(alist, long);
            ok = va_arg(alist, int*);
            *ok = handle_invalid_peer_certificate(vfy_result);
            break;
        }
        case SMTP_EV_NO_PEER_CERTIFICATE: {
            ok = va_arg(alist, int*);
            puts("SMTP_EV_NO_PEER_CERTIFICATE - accepted.");
            *ok = 1;
            break;
        }
        case SMTP_EV_WRONG_PEER_CERTIFICATE: {
            ok = va_arg(alist, int*);
            puts("SMTP_EV_WRONG_PEER_CERTIFICATE - accepted.");
            *ok = 1;
            break;
        }
        case SMTP_EV_NO_CLIENT_CERTIFICATE: {
            ok = va_arg(alist, int*);
            puts("SMTP_EV_NO_CLIENT_CERTIFICATE - accepted.");
            *ok = 1;
            break;
        }
        default:
            printf("Got event: %d - ignored.\n", event_no);
    }
    va_end(alist);
}

/* Callback to prnt the recipient status */
void print_recipient_status(smtp_recipient_t recipient, const char* mailbox,
                            void* arg)
{
    const smtp_status_t* status;

    status = smtp_recipient_status(recipient);
    std::cerr << mailbox << status->code << status->text;
}

uint16_t Smtp::sendmail(const std::string& recipient,
                        const std::string& subject, const std::string& msg)
{

    if ((Smtp::enable == false) || Smtp::host.empty() || (Smtp::port == 0) ||
        Smtp::sender.empty())
    {
        return -1;
    }

    if (Smtp::enable == false)
    {
        return -2;
    }

    auth_client_init();
    Smtp::session = smtp_create_session();
    Smtp::message = smtp_add_message(Smtp::session);

    // smtp_set_monitorcb (session, monitor_cb, stdout, 1);

    // smtp_set_header (message, "Disposition-Notification-To", NULL, NULL);

    // smtp_starttls_enable (session, Starttls_ENABLED);
    // smtp_starttls_enable (session, Starttls_REQUIRED);

    char* Toch = new char[recipient.length() + 1];
    strcpy(Toch, recipient.c_str());
    smtp_set_header(Smtp::message, "To", NULL, Toch);
    // smtp_set_header (Smtp::message, "Cc", NULL, NULL);
    // smtp_set_header (Smtp::message, "Bcc", NULL, NULL);

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);

    std::string smtpPort = std::to_string(Smtp::port);

    char* hostch = new char[host.length() + smtpPort.length() + 2];
    strcpy(hostch, (host + ":" + smtpPort).c_str());
    smtp_set_server(Smtp::session, hostch);

    Smtp::authctx = auth_create_context();
    auth_set_mechanism_flags(Smtp::authctx, AUTH_PLUGIN_PLAIN, 0);
    auth_set_interact_cb(Smtp::authctx, authinteract, NULL);

    smtp_starttls_set_password_cb(tlsinteract, NULL);
    smtp_set_eventcb(Smtp::session, event_cb, NULL);

    // if (!noauth)
    // smtp_auth_set_context (Smtp::session, Smtp::authctx);

    char* fromch = new char[sender.length() + 1];
    strcpy(fromch, sender.c_str());
    smtp_set_reverse_path(Smtp::message, fromch);

    char* subch = new char[subject.length() + 1];
    strcpy(subch, subject.c_str());
    smtp_set_header(Smtp::message, "Subject", subch);
    smtp_set_header_option(Smtp::message, "Subject", Hdr_OVERRIDE, 1);

    char* msgch = new char[msg.length() + 5];
    strcpy(msgch, ("\r\n" + msg + "\r\n").c_str());
    smtp_set_message_str(Smtp::message, msgch);

    smtp_add_recipient(Smtp::message, Toch);

    /* Recipient options set here */
    //    if (notify != Notify_NOTSET)
    //      smtp_dsn_set_notify (recipient, notify);
    //  }

    if (!smtp_start_session(Smtp::session))
    {
        std::cerr << "SMTP server problem \n";
    }
    else
    {
        Smtp::status = smtp_message_transfer_status(Smtp::message);
        std::cerr << "SMTP mail status: " << Smtp::status->text << "\n";
        smtp_enumerate_recipients(message, print_recipient_status, NULL);
    }

    smtp_destroy_session(Smtp::session);
    auth_destroy_context(Smtp::authctx);
    auth_client_exit();
    return 0;
}

std::tuple<bool, std::string, uint16_t, std::string> Smtp::getsmtpconfig()
{

    std::tuple<bool, std::string, uint16_t, std::string> smtpcfg;
    smtpcfg = make_tuple(Smtp::enable, Smtp::host, Smtp::port, Smtp::sender);
    return smtpcfg;
}

uint16_t Smtp::setsmtpconfig(const bool enable, const std::string& host,
                             const uint16_t& port, const std::string& sender)
{

    std::ofstream configFile;
    configFile.open(configFilePath, std::ios::out | std::ios::trunc);

    Json privData, jsonData;
    privData["Enabled"] = enable;
    privData["Host"] = host;
    privData["Port"] = port;
    privData["Sender"] = sender;

    jsonData["Config"] = privData;

    const auto& writeData = jsonData.dump(4);
    configFile << writeData << std::endl;

    // close the opened file.
    configFile.close();

    Smtp::enable = enable;
    Smtp::host = host;
    Smtp::port = port;
    Smtp::sender = sender;

    return 0;
}

int Smtp::initializeSmtpcfg()
{
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open())
    {
        std::cerr << "initializeSmtpcfg: Cannot open config path";
        return -1;
    }
    try
    {
        auto data = nlohmann::json::parse(configFile, nullptr);
        Json smtpConfig = data["Config"];
        Smtp::enable = smtpConfig["Enabled"];
        Smtp::host = smtpConfig["Host"];
        Smtp::port = smtpConfig["Port"];
        Smtp::sender = smtpConfig["Sender"];
    }
    catch (nlohmann::json::exception& e)
    {
        std::cerr << "initializeSmtpcfg: Error parsing config file";
        return -1;
    }
    catch (std::out_of_range& e)
    {
        std::cerr << "initializeChannelsSmtpcfg: Error invalid type";
        return -1;
    }
    return 0;
}

} // namespace manager
} // namespace alert
} // namespace mail
