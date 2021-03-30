#include "config.h"

#include "cpldimage_manager.hpp"

#include "cpldversion.hpp"
#include "cpldwatch.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <elog-errors.hpp>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <xyz/openbmc_project/Cpld/Image/error.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <string>
#include "lattice.hpp"
#include "ast-jtag.hpp"

namespace phosphor
{
namespace cpld
{
namespace manager
{

using namespace phosphor::logging;
using namespace sdbusplus::xyz::openbmc_project::Cpld::Image::Error;
namespace Cpld = phosphor::logging::xyz::openbmc_project::Cpld;
using ManifestFail = Cpld::Image::ManifestFileFailure;
using UnTarFail = Cpld::Image::UnTarFailure;
using InternalFail = Cpld::Image::InternalFailure;
using UploadImageFail = Cpld::Image::UploadImageFailure;
namespace fs = std::filesystem;

struct RemovablePath
{
    fs::path path;

    RemovablePath(const fs::path& path) : path(path)
    {}
    ~RemovablePath()
    {
        if (!path.empty())
        {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
    }
};

namespace // anonymous
{

std::vector<std::string> getSoftwareObjects(sdbusplus::bus::bus& bus)
{
    std::vector<std::string> paths;
    auto method = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                      MAPPER_INTERFACE, "GetSubTreePaths");
    method.append(CPLD_OBJPATH);
    method.append(0); // Depth 0 to search all
    method.append(std::vector<std::string>({CPLD_VERSION_BUSNAME}));
    auto reply = bus.call(method);
    reply.read(paths);
    return paths;
}

static std::string functionalImageVersion(const std::string& releaseFilePath)
{
    std::string versionKey = "VERSION_ID=";
    std::string version{};
    std::ifstream efile;
    std::string line;
    efile.open(releaseFilePath);

    while (getline(efile, line))
    {
        if (line.substr(0, versionKey.size()).find(versionKey) !=
            std::string::npos)
        {
            std::size_t pos = line.find_first_of('"') + 1;
            version = line.substr(pos, line.find_last_of('"') - pos);
            break;
        }
    }
    efile.close();

    if (version.empty())
    {
        log<level::ERR>("Error BMC current version is empty");
    }

    return version;
}

} // namespace


int Manager::cpldprocessImage(const std::string& tarFilePath)
{
    if (!fs::is_regular_file(tarFilePath))
    {
        log<level::ERR>("Error tarball does not exist",
                        entry("FILENAME=%s", tarFilePath.c_str()));
        report<ManifestFileFailure>(ManifestFail::PATH(tarFilePath.c_str()));
        return -1;
    }
    RemovablePath tarPathRemove(tarFilePath);
    fs::path tmpDirPath(std::string{CPLD_IMG_UPLOAD_DIR});
    tmpDirPath /= "imageXXXXXX";
    auto tmpDir = tmpDirPath.string();

    // Create a tmp dir to extract tarball.
    if (!mkdtemp(tmpDir.data()))
    {
        log<level::ERR>("Error in uploading the cpld image",
                        entry("ERRNO=%d", errno));
        report<InternalFailure>(InternalFail::FAIL("mkdtemp"));
        return -1;
    }

    tmpDirPath = tmpDir;
    RemovablePath tmpDirToRemove(tmpDirPath);
    fs::path manifestPath = tmpDirPath;
    manifestPath /= MANIFEST_FILE_NAME;

    // Untar tarball into the tmp dir
    auto rc = unTar(tarFilePath, tmpDirPath.string());
    if (rc < 0)
    {
        log<level::ERR>("Error occurred during untar");
        return -1;
    }

    // verify upload image
    int ret = 0;
    ret = cpldcm();
    std::ifstream erasefile("/tmp/erase");
    std::ifstream verifyfile("/tmp/verify");
    std::ifstream programfile("/tmp/program");

    if (ret == -1) 
    {
	syslog(LOG_WARNING,"AST LATTICE Device - UnKnow :\n");
	log<level::ERR>("Error CPLD device is not connected ",
				entry("ERRNO=%d", errno));
	report<UploadImageFailure>(UploadImageFail::FAIL("CPLD"));
	if (!(erasefile || verifyfile || programfile ))
		ast_jtag_close();	
        return -1;
    }

    ast_jtag_close();

    // Verify the manifest file
    if (!fs::is_regular_file(manifestPath))
    {
        log<level::ERR>("Error No manifest file",
                        entry("FILENAME=%s", tarFilePath.c_str()));
        report<ManifestFileFailure>(ManifestFail::PATH(tarFilePath.c_str()));
        return -1;
    }

    // Get version
    auto version = Version::getValue(manifestPath.string(), "version");
    if (version.empty())
    {
        log<level::ERR>("Error unable to read version from manifest file");
        report<ManifestFileFailure>(ManifestFail::PATH(tarFilePath.c_str()));
        return -1;
    }

    // Get running machine name
    std::string currMachine = Version::getBMCMachine(OS_RELEASE_FILE);
    if (currMachine.empty())
    {
        log<level::ERR>("Failed to read machine name from osRelease",
                        entry("FILENAME=%s", OS_RELEASE_FILE));
        return -1;
    }

    // Get machine name for image to be upgraded
    std::string machineStr =
        Version::getValue(manifestPath.string(), "MachineName");
    if (!machineStr.empty())
    {
        if (machineStr != currMachine)
        {
            log<level::ERR>("BMC upgrade: Machine name doesn't match",
                            entry("CURR_MACHINE=%s", currMachine.c_str()),
                            entry("NEW_MACHINE=%s", machineStr.c_str()));
            return -1;
        }
    }
    else
    {
        log<level::WARNING>("No machine name in Manifest file");
    }

    // Get purpose
    auto purposeString = Version::getValue(manifestPath.string(), "purpose");
    if (purposeString.empty())
    {
        log<level::ERR>("Error unable to read purpose from manifest file");
        report<ManifestFileFailure>(ManifestFail::PATH(tarFilePath.c_str()));
        return -1;
    }

    auto purpose = Version::VersionPurpose::Unknown;
    try
    {
        purpose = Version::convertVersionPurposeFromString(purposeString);
    }
    catch (const sdbusplus::exception::InvalidEnumString& e)
    {
        log<level::ERR>("Error: Failed to convert manifest purpose to enum."
                        " Setting to Unknown.");
    }

    // Compute id
    auto id = Version::getId(version);

    fs::path imageDirPath = std::string{CPLD_IMG_UPLOAD_DIR};
    imageDirPath /= id;

    // For CPLD erase and program URL'S
    system("/bin/echo Erase Operation is not initiated > /tmp/cplderase");
    system("/bin/echo Flashing Operation is not initiated > /tmp/cpldflash");

    // Create Version object
    auto objPath = std::string{CPLD_OBJPATH} + '/' + id;

    // This service only manages the uploaded versions, and there could be
    // active versions on D-Bus that is not managed by this service.
    // So check D-Bus if there is an existing version.
    auto allSoftwareObjs = getSoftwareObjects(bus);
    auto it =
        std::find(allSoftwareObjs.begin(), allSoftwareObjs.end(), objPath);

    const std::string uploadVersion = version.c_str();
    const std::string currentfunctionalVersion = functionalImageVersion(OS_RELEASE_FILE);
    
    if (versions.find(id) == versions.end() && it == allSoftwareObjs.end() && uploadVersion != currentfunctionalVersion )
    {
    	// Rename the temp dir to image dir
    	fs::rename(tmpDirPath, imageDirPath);
    	// Clear the path, so it does not attemp to remove a non-existing path
    	tmpDirToRemove.path.clear();
        auto versionPtr = std::make_unique<Version>(
            bus, objPath, version, purpose, imageDirPath.string(), currentfunctionalVersion,
            std::bind(&Manager::erase, this, std::placeholders::_1));
        versionPtr->deleteObject =
            std::make_unique<phosphor::cpld::manager::Delete>(bus, objPath,
                                                                  *versionPtr);
        versions.insert(std::make_pair(id, std::move(versionPtr)));
    }
    else
    {
    	if (fs::exists(tmpDirPath))
	{
		//remove the tmpdir because that image is already present
	        fs::remove_all(tmpDirPath);
	}
	// Clear the path, so it does not attemp to remove a non-existing path
    	tmpDirToRemove.path.clear();
	log<level::INFO>("Software Object with the same version already exists",
                         entry("VERSION_ID=%s", id.c_str()));
    }
    return 0;
}

void Manager::erase(std::string entryId)
{
    auto it = versions.find(entryId);
    if (it == versions.end())
    {
        return;
    }

    if (it->second->isFunctional())
    {
        log<level::ERR>(("Error: Version " + entryId +
                         " is currently running on the BMC."
                         " Unable to remove.")
                            .c_str());
        return;
    }

    // Delete image dir
    fs::path imageDirPath = (*(it->second)).path();
    if (fs::exists(imageDirPath))
    {
        fs::remove_all(imageDirPath);
    }
    this->versions.erase(entryId);
}

int Manager::unTar(const std::string& tarFilePath,
                   const std::string& extractDirPath)
{
    if (tarFilePath.empty())
    {
        log<level::ERR>("Error TarFilePath is empty");
        report<UnTarFailure>(UnTarFail::PATH(tarFilePath.c_str()));
        return -1;
    }
    if (extractDirPath.empty())
    {
        log<level::ERR>("Error ExtractDirPath is empty");
        report<UnTarFailure>(UnTarFail::PATH(extractDirPath.c_str()));
        return -1;
    }

    log<level::INFO>("Untaring", entry("FILENAME=%s", tarFilePath.c_str()),
                     entry("EXTRACTIONDIR=%s", extractDirPath.c_str()));
    int status = 0;
    pid_t pid = fork();

    if (pid == 0)
    {
        // child process
        execl("/bin/tar", "tar", "-xf", tarFilePath.c_str(), "-C",
              extractDirPath.c_str(), (char*)0);
        // execl only returns on fail
        log<level::ERR>("Failed to execute untar file",
                        entry("FILENAME=%s", tarFilePath.c_str()));
        report<UnTarFailure>(UnTarFail::PATH(tarFilePath.c_str()));
        return -1;
    }
    else if (pid > 0)
    {
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status))
        {
            log<level::ERR>("Failed to untar file",
                            entry("FILENAME=%s", tarFilePath.c_str()));
            report<UnTarFailure>(UnTarFail::PATH(tarFilePath.c_str()));
            return -1;
        }
    }
    else
    {
        log<level::ERR>("fork() failed.");
        report<UnTarFailure>(UnTarFail::PATH(tarFilePath.c_str()));
        return -1;
    }

    return 0;
}

} // namespace manager
} // namespace cpld
} // namespace phosphor
