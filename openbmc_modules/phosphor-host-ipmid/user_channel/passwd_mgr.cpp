/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "passwd_mgr.hpp"

#include "file.hpp"
#include "shadowlock.hpp"

#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <phosphor-logging/log.hpp>

namespace ipmi
{

static const char* passwdFileName = "/etc/ipmi_pass";
static const char* encryptKeyFileName = "/etc/key_file";
static const size_t maxKeySize = 8;

constexpr mode_t modeMask =
    (S_ISUID | S_ISGID | S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO);

#define META_PASSWD_SIG "=OPENBMC="

/*
 * Meta data struct for encrypted password file
 */
struct MetaPassStruct
{
    char signature[10];
    unsigned char reseved[2];
    size_t hashSize;
    size_t ivSize;
    size_t dataSize;
    size_t padSize;
    size_t macSize;
};

using namespace phosphor::logging;

PasswdMgr::PasswdMgr()
{
    restrictFilesPermission();
    initPasswordMap();
}

void PasswdMgr::restrictFilesPermission(void)
{
    struct stat st = {};
    // Restrict file permission to owner read & write
    if (stat(passwdFileName, &st) == 0)
    {
        if ((st.st_mode & modeMask) != (S_IRUSR | S_IWUSR))
        {
            chmod(passwdFileName, S_IRUSR | S_IWUSR);
        }
    }

    if (stat(encryptKeyFileName, &st) == 0)
    {
        if ((st.st_mode & modeMask) != (S_IRUSR | S_IWUSR))
        {
            chmod(encryptKeyFileName, S_IRUSR | S_IWUSR);
        }
    }
}

std::string PasswdMgr::getPasswdByUserName(const std::string& userName)
{
    checkAndReload();
    auto iter = passwdMapList.find(userName);
    if (iter == passwdMapList.end())
    {
        return std::string();
    }
    return iter->second;
}

int PasswdMgr::updateUserEntry(const std::string& userName,
                               const std::string& newUserName)
{
    std::time_t updatedTime = getUpdatedFileTime();
    // Check file time stamp to know passwdMapList is up-to-date.
    // If not up-to-date, then updatePasswdSpecialFile will read and
    // check the user entry existance.
    if (fileLastUpdatedTime == updatedTime && updatedTime != -EIO)
    {
        if (passwdMapList.find(userName) == passwdMapList.end())
        {
            log<level::DEBUG>("User not found");
            return 0;
        }
    }

    // Write passwdMap to Encryted file
    if (updatePasswdSpecialFile(userName, newUserName) != 0)
    {
        log<level::DEBUG>("Passwd file update failed");
        return -EIO;
    }

    log<level::DEBUG>("Passwd file updated successfully");
    return 0;
}

void PasswdMgr::checkAndReload(void)
{
    std::time_t updatedTime = getUpdatedFileTime();
    if (fileLastUpdatedTime != updatedTime && updatedTime != -1)
    {
        log<level::DEBUG>("Reloading password map list");
        passwdMapList.clear();
        initPasswordMap();
    }
}

int PasswdMgr::encryptDecryptData(bool doEncrypt, const EVP_CIPHER* cipher,
                                  uint8_t* key, size_t keyLen, uint8_t* iv,
                                  size_t ivLen, uint8_t* inBytes,
                                  size_t inBytesLen, uint8_t* mac,
                                  size_t* macLen, unsigned char* outBytes,
                                  size_t* outBytesLen)
{
    if (cipher == NULL || key == NULL || iv == NULL || inBytes == NULL ||
        outBytes == NULL || mac == NULL || inBytesLen == 0 ||
        (size_t)EVP_CIPHER_key_length(cipher) > keyLen ||
        (size_t)EVP_CIPHER_iv_length(cipher) > ivLen)
    {
        log<level::DEBUG>("Error Invalid Inputs");
        return -EINVAL;
    }

    if (!doEncrypt)
    {
        // verify MAC before decrypting the data.
        std::array<uint8_t, EVP_MAX_MD_SIZE> calMac;
        size_t calMacLen = calMac.size();
        // calculate MAC for the encrypted message.
        if (NULL == HMAC(EVP_sha256(), key, keyLen, inBytes, inBytesLen,
                         calMac.data(),
                         reinterpret_cast<unsigned int*>(&calMacLen)))
        {
            log<level::DEBUG>("Error: Failed to calculate MAC");
            return -EIO;
        }
        if (!((calMacLen == *macLen) &&
              (std::memcmp(calMac.data(), mac, calMacLen) == 0)))
        {
            log<level::DEBUG>("Authenticated message doesn't match");
            return -EBADMSG;
        }
    }

    std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)> ctx(
        EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    EVP_CIPHER_CTX_set_padding(ctx.get(), 1);

    // Set key & IV
    int retval = EVP_CipherInit_ex(ctx.get(), cipher, NULL, key, iv,
                                   static_cast<int>(doEncrypt));
    if (!retval)
    {
        log<level::DEBUG>("EVP_CipherInit_ex failed",
                          entry("RET_VAL=%d", retval));
        return -EIO;
    }

    int outLen = 0, outEVPLen = 0;
    if ((retval = EVP_CipherUpdate(ctx.get(), outBytes + outLen, &outEVPLen,
                                   inBytes, inBytesLen)))
    {
        outLen += outEVPLen;
        if ((retval =
                 EVP_CipherFinal(ctx.get(), outBytes + outLen, &outEVPLen)))
        {
            outLen += outEVPLen;
            *outBytesLen = outLen;
        }
        else
        {
            log<level::DEBUG>("EVP_CipherFinal fails",
                              entry("RET_VAL=%d", retval));
            return -EIO;
        }
    }
    else
    {
        log<level::DEBUG>("EVP_CipherUpdate fails",
                          entry("RET_VAL=%d", retval));
        return -EIO;
    }

    if (doEncrypt)
    {
        // Create MAC for the encrypted message
        if (NULL == HMAC(EVP_sha256(), key, keyLen, outBytes, *outBytesLen, mac,
                         reinterpret_cast<unsigned int*>(macLen)))
        {
            log<level::DEBUG>("Failed to create authentication");
            return -EIO;
        }
    }
    return 0;
}

void PasswdMgr::initPasswordMap(void)
{
    phosphor::user::shadow::Lock lock();
    std::vector<uint8_t> dataBuf;

    if (readPasswdFileData(dataBuf) != 0)
    {
        log<level::DEBUG>("Error in reading the encrypted pass file");
        return;
    }

    if (dataBuf.size() != 0)
    {
        // populate the user list with password
        char* outPtr = reinterpret_cast<char*>(dataBuf.data());
        char* nToken = NULL;
        char* linePtr = strtok_r(outPtr, "\n", &nToken);
        size_t lineSize = 0;
        while (linePtr != NULL)
        {
            size_t userEPos = 0;
            std::string lineStr(linePtr);
            if ((userEPos = lineStr.find(":")) != std::string::npos)
            {
                lineSize = lineStr.size();
                passwdMapList.emplace(
                    lineStr.substr(0, userEPos),
                    lineStr.substr(userEPos + 1, lineSize - (userEPos + 1)));
            }
            linePtr = strtok_r(NULL, "\n", &nToken);
        }
    }

    // Update the timestamp
    fileLastUpdatedTime = getUpdatedFileTime();
    return;
}

int PasswdMgr::readPasswdFileData(std::vector<uint8_t>& outBytes)
{
    std::array<uint8_t, maxKeySize> keyBuff;
    std::ifstream keyFile(encryptKeyFileName, std::ios::in | std::ios::binary);
    if (!keyFile.is_open())
    {
        log<level::DEBUG>("Error in opening encryption key file");
        return -EIO;
    }
    keyFile.read(reinterpret_cast<char*>(keyBuff.data()), keyBuff.size());
    if (keyFile.fail())
    {
        log<level::DEBUG>("Error in reading encryption key file");
        return -EIO;
    }

    std::ifstream passwdFile(passwdFileName, std::ios::in | std::ios::binary);
    if (!passwdFile.is_open())
    {
        log<level::DEBUG>("Error in opening ipmi password file");
        return -EIO;
    }

    // calculate file size and read the data
    passwdFile.seekg(0, std::ios::end);
    ssize_t fileSize = passwdFile.tellg();
    passwdFile.seekg(0, std::ios::beg);
    std::vector<uint8_t> input(fileSize);
    passwdFile.read(reinterpret_cast<char*>(input.data()), fileSize);
    if (passwdFile.fail())
    {
        log<level::DEBUG>("Error in reading encryption key file");
        return -EIO;
    }

    // verify the signature first
    MetaPassStruct* metaData = reinterpret_cast<MetaPassStruct*>(input.data());
    if (std::strncmp(metaData->signature, META_PASSWD_SIG,
                     sizeof(metaData->signature)))
    {
        log<level::DEBUG>("Error signature mismatch in password file");
        return -EBADMSG;
    }

    size_t inBytesLen = metaData->dataSize + metaData->padSize;
    // If data is empty i.e no password map then return success
    if (inBytesLen == 0)
    {
        log<level::DEBUG>("Empty password file");
        return 0;
    }

    // compute the key needed to decrypt
    std::array<uint8_t, EVP_MAX_KEY_LENGTH> key;
    auto keyLen = key.size();
    if (NULL == HMAC(EVP_sha256(), keyBuff.data(), keyBuff.size(),
                     input.data() + sizeof(*metaData), metaData->hashSize,
                     key.data(), reinterpret_cast<unsigned int*>(&keyLen)))
    {
        log<level::DEBUG>("Failed to create MAC for authentication");
        return -EIO;
    }

    // decrypt the data
    uint8_t* iv = input.data() + sizeof(*metaData) + metaData->hashSize;
    size_t ivLen = metaData->ivSize;
    uint8_t* inBytes = iv + ivLen;
    uint8_t* mac = inBytes + inBytesLen;
    size_t macLen = metaData->macSize;

    size_t outBytesLen = 0;
    // Resize to actual data size
    outBytes.resize(inBytesLen + EVP_MAX_BLOCK_LENGTH);
    if (encryptDecryptData(false, EVP_aes_128_cbc(), key.data(), keyLen, iv,
                           ivLen, inBytes, inBytesLen, mac, &macLen,
                           outBytes.data(), &outBytesLen) != 0)
    {
        log<level::DEBUG>("Error in decryption");
        return -EIO;
    }
    // Resize the vector to outBytesLen
    outBytes.resize(outBytesLen);

    OPENSSL_cleanse(key.data(), keyLen);
    OPENSSL_cleanse(iv, ivLen);

    return 0;
}

int PasswdMgr::updatePasswdSpecialFile(const std::string& userName,
                                       const std::string& newUserName)
{
    phosphor::user::shadow::Lock lock();

    size_t bytesWritten = 0;
    size_t inBytesLen = 0;
    size_t isUsrFound = false;
    const EVP_CIPHER* cipher = EVP_aes_128_cbc();
    std::vector<uint8_t> dataBuf;

    // Read the encrypted file and get the file data
    // Check user existance and return if not exist.
    if (readPasswdFileData(dataBuf) != 0)
    {
        log<level::DEBUG>("Error in reading the encrypted pass file");
        return -EIO;
    }

    if (dataBuf.size() != 0)
    {
        inBytesLen =
            dataBuf.size() + newUserName.size() + EVP_CIPHER_block_size(cipher);
    }

    std::vector<uint8_t> inBytes(inBytesLen);
    if (inBytesLen != 0)
    {
        char* outPtr = reinterpret_cast<char*>(dataBuf.data());
        char* nToken = NULL;
        char* linePtr = strtok_r(outPtr, "\n", &nToken);
        while (linePtr != NULL)
        {
            size_t userEPos = 0;

            std::string lineStr(linePtr);
            if ((userEPos = lineStr.find(":")) != std::string::npos)
            {
                if (userName.compare(lineStr.substr(0, userEPos)) == 0)
                {
                    isUsrFound = true;
                    if (!newUserName.empty())
                    {
                        bytesWritten += std::snprintf(
                            reinterpret_cast<char*>(&inBytes[0]) + bytesWritten,
                            (inBytesLen - bytesWritten), "%s%s\n",
                            newUserName.c_str(),
                            lineStr.substr(userEPos, lineStr.size()).data());
                    }
                }
                else
                {
                    bytesWritten += std::snprintf(
                        reinterpret_cast<char*>(&inBytes[0]) + bytesWritten,
                        (inBytesLen - bytesWritten), "%s\n", lineStr.data());
                }
            }
            linePtr = strtok_r(NULL, "\n", &nToken);
        }
        inBytesLen = bytesWritten;
    }
    if (!isUsrFound)
    {
        log<level::DEBUG>("User doesn't exist");
        return 0;
    }

    // Read the key buff from key file
    std::array<uint8_t, maxKeySize> keyBuff;
    std::ifstream keyFile(encryptKeyFileName, std::ios::in | std::ios::binary);
    if (!keyFile.good())
    {
        log<level::DEBUG>("Error in opening encryption key file");
        return -EIO;
    }
    keyFile.read(reinterpret_cast<char*>(keyBuff.data()), keyBuff.size());
    if (keyFile.fail())
    {
        log<level::DEBUG>("Error in reading encryption key file");
        return -EIO;
    }
    keyFile.close();

    // Read the original passwd file mode
    struct stat st = {};
    if (stat(passwdFileName, &st) != 0)
    {
        log<level::DEBUG>("Error in getting password file fstat()");
        return -EIO;
    }

    // Create temporary file for write
    std::string pwdFile(passwdFileName);
    std::vector<char> tempFileName(pwdFile.begin(), pwdFile.end());
    std::vector<char> fileTemplate = {'_', '_', 'X', 'X', 'X',
                                      'X', 'X', 'X', '\0'};
    tempFileName.insert(tempFileName.end(), fileTemplate.begin(),
                        fileTemplate.end());
    int fd = mkstemp((char*)tempFileName.data());
    if (fd == -1)
    {
        log<level::DEBUG>("Error creating temp file");
        return -EIO;
    }

    std::string strTempFileName(tempFileName.data());
    // Open the temp file for writing from provided fd
    // By "true", remove it at exit if still there.
    // This is needed to cleanup the temp file at exception
    phosphor::user::File temp(fd, strTempFileName, "w", true);
    if ((temp)() == NULL)
    {
        close(fd);
        log<level::DEBUG>("Error creating temp file");
        return -EIO;
    }

    // Set the file mode as read-write for owner only
    if (fchmod(fileno((temp)()), S_IRUSR | S_IWUSR) < 0)
    {
        log<level::DEBUG>("Error setting fchmod for temp file");
        return -EIO;
    }

    const EVP_MD* digest = EVP_sha256();
    size_t hashLen = EVP_MD_block_size(digest);
    std::vector<uint8_t> hash(hashLen);
    size_t ivLen = EVP_CIPHER_iv_length(cipher);
    std::vector<uint8_t> iv(ivLen);
    std::array<uint8_t, EVP_MAX_KEY_LENGTH> key;
    size_t keyLen = key.size();
    std::array<uint8_t, EVP_MAX_MD_SIZE> mac;
    size_t macLen = mac.size();

    // Create random hash and generate hash key which will be used for
    // encryption.
    if (RAND_bytes(hash.data(), hashLen) != 1)
    {
        log<level::DEBUG>("Hash genertion failed, bailing out");
        return -EIO;
    }
    if (NULL == HMAC(digest, keyBuff.data(), keyBuff.size(), hash.data(),
                     hashLen, key.data(),
                     reinterpret_cast<unsigned int*>(&keyLen)))
    {
        log<level::DEBUG>("Failed to create MAC for authentication");
        return -EIO;
    }

    // Generate IV values
    if (RAND_bytes(iv.data(), ivLen) != 1)
    {
        log<level::DEBUG>("UV genertion failed, bailing out");
        return -EIO;
    }

    // Encrypt the input data
    std::vector<uint8_t> outBytes(inBytesLen + EVP_MAX_BLOCK_LENGTH);
    size_t outBytesLen = 0;
    if (inBytesLen != 0)
    {
        if (encryptDecryptData(true, EVP_aes_128_cbc(), key.data(), keyLen,
                               iv.data(), ivLen, inBytes.data(), inBytesLen,
                               mac.data(), &macLen, outBytes.data(),
                               &outBytesLen) != 0)
        {
            log<level::DEBUG>("Error while encrypting the data");
            return -EIO;
        }
        outBytes[outBytesLen] = 0;
    }
    OPENSSL_cleanse(key.data(), keyLen);

    // Update the meta password structure.
    MetaPassStruct metaData = {META_PASSWD_SIG, {0, 0}, 0, 0, 0, 0, 0};
    metaData.hashSize = hashLen;
    metaData.ivSize = ivLen;
    metaData.dataSize = bytesWritten;
    metaData.padSize = outBytesLen - bytesWritten;
    metaData.macSize = macLen;

    if (fwrite(&metaData, 1, sizeof(metaData), (temp)()) != sizeof(metaData))
    {
        log<level::DEBUG>("Error in writing meta data");
        return -EIO;
    }

    if (fwrite(&hash[0], 1, hashLen, (temp)()) != hashLen)
    {
        log<level::DEBUG>("Error in writing hash data");
        return -EIO;
    }

    if (fwrite(&iv[0], 1, ivLen, (temp)()) != ivLen)
    {
        log<level::DEBUG>("Error in writing IV data");
        return -EIO;
    }

    if (fwrite(&outBytes[0], 1, outBytesLen, (temp)()) != outBytesLen)
    {
        log<level::DEBUG>("Error in writing encrypted data");
        return -EIO;
    }

    if (fwrite(&mac[0], 1, macLen, (temp)()) != macLen)
    {
        log<level::DEBUG>("Error in writing MAC data");
        return -EIO;
    }

    if (fflush((temp)()))
    {
        log<level::DEBUG>(
            "File fflush error while writing entries to special file");
        return -EIO;
    }

    OPENSSL_cleanse(iv.data(), ivLen);

    // Rename the tmp  file to actual file
    if (std::rename(strTempFileName.data(), passwdFileName) != 0)
    {
        log<level::DEBUG>("Failed to rename tmp file to ipmi-pass");
        return -EIO;
    }

    return 0;
}

std::time_t PasswdMgr::getUpdatedFileTime()
{
    struct stat fileStat = {};
    if (stat(passwdFileName, &fileStat) != 0)
    {
        log<level::DEBUG>("Error - Getting passwd file time stamp");
        return -EIO;
    }
    return fileStat.st_mtime;
}

} // namespace ipmi
