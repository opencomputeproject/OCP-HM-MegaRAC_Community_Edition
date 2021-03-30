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
#pragma once
#include <openssl/evp.h>

#include <ctime>
#include <string>
#include <unordered_map>
#include <vector>

namespace ipmi
{

class PasswdMgr
{
  public:
    ~PasswdMgr() = default;
    PasswdMgr(const PasswdMgr&) = delete;
    PasswdMgr& operator=(const PasswdMgr&) = delete;
    PasswdMgr(PasswdMgr&&) = delete;
    PasswdMgr& operator=(PasswdMgr&&) = delete;

    /** @brief Constructs user password list
     *
     */
    PasswdMgr();

    /** @brief Get password for the user
     *
     *  @param[in] userName - user name
     *
     * @return password string. will return empty string, if unable to locate
     * the user
     */
    std::string getPasswdByUserName(const std::string& userName);

    /** @brief Update / clear  username and password entry for the specified
     * user
     *
     *  @param[in] userName - user name that has to be renamed / deleted
     *  @param[in] newUserName - new user name. If empty, userName will be
     *   deleted.
     *
     * @return error response
     */
    int updateUserEntry(const std::string& userName,
                        const std::string& newUserName);

  private:
    using UserName = std::string;
    using Password = std::string;
    std::unordered_map<UserName, Password> passwdMapList;
    std::time_t fileLastUpdatedTime;

    /** @brief restrict file permission
     *
     */
    void restrictFilesPermission(void);
    /** @brief check timestamp and reload password map if required
     *
     */
    void checkAndReload(void);
    /** @brief initializes passwdMapList by reading the encrypted file
     *
     * Initializes the passwordMapList members after decrypting the
     * password file. passwordMapList will be used further in IPMI
     * authentication.
     */
    void initPasswordMap(void);

    /** @brief Function to read the encrypted password file data
     *
     *  @param[out] outBytes - vector to hold decrypted password file data
     *
     * @return error response
     */
    int readPasswdFileData(std::vector<uint8_t>& outBytes);
    /** @brief  Updates special password file by clearing the password entry
     *  for the user specified.
     *
     *  @param[in] userName - user name that has to be renamed / deleted
     *  @param[in] newUserName - new user name. If empty, userName will be
     *   deleted.
     *
     * @return error response
     */
    int updatePasswdSpecialFile(const std::string& userName,
                                const std::string& newUserName);
    /** @brief encrypts or decrypt the data provided
     *
     *  @param[in] doEncrypt - do encrypt if set to true, else do decrypt.
     *  @param[in] cipher - cipher to be used
     *  @param[in] key - pointer to the key
     *  @param[in] keyLen - Length of the key to be used
     *  @param[in] iv - pointer to initialization vector
     *  @param[in] ivLen - Length of the iv
     *  @param[in] inBytes - input data to be encrypted / decrypted
     *  @param[in] inBytesLen - input size to be encrypted / decrypted
     *  @param[in] mac - message authentication code - to figure out corruption
     *  @param[in] macLen - size of MAC
     *  @param[in] outBytes - ptr to store output bytes
     *  @param[in] outBytesLen - outbut data length.
     *
     * @return error response
     */
    int encryptDecryptData(bool doEncrypt, const EVP_CIPHER* cipher,
                           uint8_t* key, size_t keyLen, uint8_t* iv,
                           size_t ivLen, uint8_t* inBytes, size_t inBytesLen,
                           uint8_t* mac, size_t* macLen, uint8_t* outBytes,
                           size_t* outBytesLen);

    /** @brief  returns updated file time of passwd file entry.
     *
     * @return timestamp or -1 for error.
     */
    std::time_t getUpdatedFileTime();
};

} // namespace ipmi
