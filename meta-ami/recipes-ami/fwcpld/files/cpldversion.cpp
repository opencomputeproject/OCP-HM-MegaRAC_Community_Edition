#include "config.h"

#include "cpldversion.hpp"

#include "xyz/openbmc_project/Common/error.hpp"

#include <openssl/sha.h>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <syslog.h>
#include <unistd.h>

#include "lattice.hpp"
#include "ast-jtag.hpp"

struct cpld_dev_info *cur_dev;
xfer_mode mode = HW_MODE;

char in_name[100] = "/tmp/cpld/8bf6d1de/mxo2_7000.jed";
char dev_name[100] = "/dev/1e6e4000.jtag";

namespace phosphor
{
namespace cpld
{
namespace manager
{

std::string cpld_file("Firmware Image is not Present");
using namespace phosphor::logging;
using Argument = xyz::openbmc_project::Common::InvalidArgument;
using namespace sdbusplus::xyz::openbmc_project::Common::Error;

std::string Version::getValue(const std::string& manifestFilePath,
                              std::string key)
{
    key = key + "=";
    auto keySize = key.length();

    if (manifestFilePath.empty())
    {
        log<level::ERR>("Error MANIFESTFilePath is empty");
        elog<InvalidArgument>(
            Argument::ARGUMENT_NAME("manifestFilePath"),
            Argument::ARGUMENT_VALUE(manifestFilePath.c_str()));
    }

    std::string value{};
    std::ifstream efile;
    std::string line;
    efile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                     std::ifstream::eofbit);

    // Too many GCC bugs (53984, 66145) to do this the right way...
    try
    {
        efile.open(manifestFilePath);
        while (getline(efile, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                // If the manifest has CRLF line terminators, e.g. is created on
                // Windows, the line will contain \r at the end, remove it.
                line.pop_back();
            }
            if (line.compare(0, keySize, key) == 0)
            {
                value = line.substr(keySize);
                break;
            }
        }
        efile.close();
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Error in reading MANIFEST file");
    }

    return value;
}

std::string Version::getId(const std::string& version)
{

    if (version.empty())
    {
        log<level::ERR>("Error version is empty");
        elog<InvalidArgument>(Argument::ARGUMENT_NAME("Version"),
                              Argument::ARGUMENT_VALUE(version.c_str()));
    }

    unsigned char digest[SHA512_DIGEST_LENGTH];
    SHA512_CTX ctx;
    SHA512_Init(&ctx);
    SHA512_Update(&ctx, version.c_str(), strlen(version.c_str()));
    SHA512_Final(digest, &ctx);
    char mdString[SHA512_DIGEST_LENGTH * 2 + 1];
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
    {
        snprintf(&mdString[i * 2], 3, "%02x", (unsigned int)digest[i]);
    }

    // Only need 8 hex digits.
    std::string hexId = std::string(mdString);
    return (hexId.substr(0, 8));
}

std::string Version::getBMCMachine(const std::string& releaseFilePath)
{
    std::string machineKey = "OPENBMC_TARGET_MACHINE=";
    std::string machine{};
    std::ifstream efile(releaseFilePath);
    std::string line;

    while (getline(efile, line))
    {
        if (line.substr(0, machineKey.size()).find(machineKey) !=
            std::string::npos)
        {
            std::size_t pos = line.find_first_of('"') + 1;
            machine = line.substr(pos, line.find_last_of('"') - pos);
            break;
        }
    }

    if (machine.empty())
    {
        log<level::ERR>("Unable to find OPENBMC_TARGET_MACHINE");
        elog<InternalFailure>();
    }

    return machine;
}

std::string Version::getBMCVersion(const std::string& releaseFilePath)
{
    std::string versionKey = "VERSION_ID=";
    std::string versionValue{};
    std::string version{};
    std::ifstream efile;
    std::string line;
    efile.open(releaseFilePath);

    while (getline(efile, line))
    {
        if (line.substr(0, versionKey.size()).find(versionKey) !=
            std::string::npos)
        {
            // Support quoted and unquoted values
            // 1. Remove the versionKey so that we process the value only.
            versionValue = line.substr(versionKey.size());

            // 2. Look for a starting quote, then increment the position by 1 to
            //    skip the quote character. If no quote is found,
            //    find_first_of() returns npos (-1), which by adding +1 sets pos
            //    to 0 (beginning of unquoted string).
            std::size_t pos = versionValue.find_first_of('"') + 1;

            // 3. Look for ending quote, then decrease the position by pos to
            //    get the size of the string up to before the ending quote. If
            //    no quote is found, find_last_of() returns npos (-1), and pos
            //    is 0 for the unquoted case, so substr() is called with a len
            //    parameter of npos (-1) which according to the documentation
            //    indicates to use all characters until the end of the string.
            version =
                versionValue.substr(pos, versionValue.find_last_of('"') - pos);
            break;
        }
    }
    efile.close();

    if (version.empty())
    {
        log<level::ERR>("Error BMC current version is empty");
        elog<InternalFailure>();
    }

    return version;
}

bool Version::isFunctional()
{
    return versionStr == getBMCVersion(OS_RELEASE_FILE);
}

void Delete::delete_()
{
    if (parent.eraseCallback)
    {
        parent.eraseCallback(parent.getId(parent.version()));
    }
}


void *call_cpldverify(void* arg)
{

        FILE *fp_in;
        fp_in = fopen(in_name, "rb");

        if (!fp_in) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n", in_name, errno, strerror(errno));
                goto OUT;
        }

        syslog(LOG_WARNING,"Verify : JEDEC file %s\n", in_name);
	cur_dev->cpld_verify(fp_in);
        fclose(fp_in);

OUT:
	system("/bin/rm -rf /tmp/verify");

        ast_jtag_close();
	return NULL; 
}

std::string Version::cpldverify()
{
	int ret,rc;
	std::string verify_success("CPLD Verify Operation is Initiated");
        pthread_t verify_thread;
        ret = cpldcm();

	if (ret < 0){
		std::string status;
		status = cpldstatus();
		return status;
	}

	std::ifstream cpldfile(in_name);
        if (!cpldfile) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n", in_name, errno, strerror(errno));
                ast_jtag_close();
                return cpld_file;
        }

	system("/bin/touch /tmp/verify");

        rc = pthread_create(&verify_thread, NULL,call_cpldverify, NULL);
        if (rc){
                ast_jtag_close();
                exit(-1);
        }
	return verify_success;
}



void *call_cplderase(void* arg)
{

	int ret = cur_dev->cpld_erase();

	if(ret == 0)
		system("/bin/echo CPLD Device Erased Successfully > /tmp/cplderase");
	else
		system("/bin/echo CPLD Device Erased Failed > /tmp/cplderase");
	
	ast_jtag_close();
	system("/bin/rm -rf /tmp/erase");
	return NULL; 
}


std::string Version::cplderase()
{
	int ret,rc;
	std::string erase_success("CPLD Erase Operation is Initiated");

        pthread_t erase_thread;

	ret = cpldcm();
	if (ret < 0){
		std::string status;
		status = cpldstatus();
		return status;
	}
		
	system("/bin/touch /tmp/erase");
        rc = pthread_create(&erase_thread, NULL,call_cplderase, NULL);
	system("/bin/echo Erase Operation Initiated > /tmp/cplderase");
        if (rc){
		ast_jtag_close();
                exit(-1);
        }
	return erase_success;
}


void *call_cpldflash(void* arg)
{
	
        FILE *fp_in;
        int ret;
	fp_in = fopen(in_name, "rb");

        if (!fp_in) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n", in_name, errno, strerror(errno));
                goto OUT;
        }
	ret = cur_dev->cpld_program(fp_in);

	if(ret == 0)
		system("/bin/echo CPLD Device Flashed Successfully > /tmp/cpldflash");
	else
		system("/bin/echo CPLD Device Flashed Failed > /tmp/cpldflash");

        fclose(fp_in);
OUT:
	system("/bin/rm -rf /tmp/program");

        ast_jtag_close();
	return NULL; 
}

std::string Version::cpldprogram()
{
	int ret,rc;

	std::string prg_success("CPLD Program Operation is Initiated");

        pthread_t flash_thread;
	ret = cpldcm();


	if (ret < 0){
		std::string status;
		status = cpldstatus();
		return status;
	}

	std::ifstream cpldfile(in_name);
        if (!cpldfile) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n", in_name, errno, strerror(errno));
                ast_jtag_close();
                return cpld_file;
        }

	
	system("/bin/touch /tmp/program");
        rc = pthread_create(&flash_thread, NULL,call_cpldflash, NULL);
	system("/bin/echo Flashing Operation Initiated > /tmp/cpldflash");
        if (rc){
		ast_jtag_close();
                exit(-1);
        }
	return prg_success;

}

std::string Version::cpldstatus()
{

	std::string str("CPLD Erase Operation is Running");
	std::string str1("CPLD Verify Operation is Running");
	std::string str2("CPLD Program Operation is Running");
	std::string nooperation("No CPLD Operation Initiated");
	std::ifstream erasefile("/tmp/erase");
	std::ifstream verifyfile("/tmp/verify");
	std::ifstream programfile("/tmp/program");

        if (erasefile)
                return str;
        else if (verifyfile)
                return str1;
        else if (programfile)
                return str2;
	return nooperation;
}

std::string Version::erasestatus()
{
	std::string line;
	std::string output;
	std::ifstream cplderasefile("/tmp/cplderase");
	if (cplderasefile.is_open())
	{
		while (getline(cplderasefile,line))
		{
			output += line;
		}
		cplderasefile.close();
	}	
	return output;
}

std::string Version::programstatus()
{
	std::string line;
	std::string output;
	std::ifstream cpldflashfile("/tmp/cpldflash");
	if (cpldflashfile.is_open())
	{
		while (getline(cpldflashfile,line))
		{
			output += line;
		}
		cpldflashfile.close();
	}
	return output;
}

} // namespace manager
} // namespace cpld
} // namespace phosphor
