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

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <security/pam_modutil.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

/*
 * This module is intended to save password of  special group user
 *
 */

#define MAX_SPEC_GRP_PASS_LENGTH 20
#define MAX_SPEC_GRP_USER_LENGTH 16
#define MAX_KEY_SIZE 8
#define DEFAULT_SPEC_PASS_FILE "/etc/ipmi_pass"
#define META_PASSWD_SIG "=OPENBMC="

/*
 * Meta data struct for storing the encrypted password file
 * Note: Followed by this structure, the real data of hash, iv, encrypted data
 * with pad and mac are stored.
 * Decrypted data will hold user name & password for every new line with format
 * like <user name>:<password>\n
 */
typedef struct metapassstruct {
	char signature[10];
	unsigned char reseved[2];
	size_t hashsize;
	size_t ivsize;
	size_t datasize;
	size_t padsize;
	size_t macsize;
} metapassstruct;

/**
 * @brief to acquire lock for atomic operation
 * Internally uses lckpwdf to acquire the lock. Tries to acquire the lock
 * using lckpwdf() in interval of 1ms, with maximum of 100 attempts.
 *
 * @return PAM_SUCCESS for success / PAM_AUTHTOK_LOCK_BUSY for failure
 */
int lock_pwdf(void)
{
	int i;
	int retval;

	i = 0;
	while ((retval = lckpwdf()) != 0 && i < 100) {
		usleep(1000);
		i++;
	}
	if (retval != 0) {
		return PAM_AUTHTOK_LOCK_BUSY;
	}
	return PAM_SUCCESS;
}

/**
 * @brief unlock the acquired lock
 * Internally uses ulckpwdf to release the lock
 */
void unlock_pwdf(void)
{
	ulckpwdf();
}

/**
 * @brief to get argument value of option
 * Function to get the value of argument options.
 *
 * @param[in] pamh - pam handle
 * @param[in] option - argument option to which value has to returned
 * @param[in] argc - argument count
 * @param[in] argv - array of arguments
 */
static const char *get_option(const pam_handle_t *pamh, const char *option,
			      int argc, const char **argv)
{
	int i;
	size_t len;

	len = strlen(option);

	for (i = 0; i < argc; ++i) {
		if (strncmp(option, argv[i], len) == 0) {
			if (argv[i][len] == '=') {
				return &argv[i][len + 1];
			}
		}
	}
	return NULL;
}

/**
 * @brief encrypt or decrypt function
 * Function which will do the encryption or decryption of the data.
 *
 * @param[in] pamh - pam handle.
 * @param[in] isencrypt - encrypt or decrypt option.
 * @param[in] cipher - EVP_CIPHER to be used
 * @param[in] key - key which has to be used in EVP_CIPHER api's.
 * @param[in] keylen - Length of the key.
 * @param[in] iv - Initialization vector data, used along with key
 * @param[in] ivlen - Length of IV.
 * @param[in] inbytes - buffer which has to be encrypted or decrypted.
 * @param[in] inbyteslen - length of input buffer.
 * @param[in] outbytes - buffer to store decrypted or encrypted data.
 * @param[in] outbyteslen - length of output buffer
 * @param[in/out] mac - checksum to cross verify. Will be verified for decrypt
 * and returns for encrypt.
 * @param[in/out] maclen - length of checksum
 * @return - 0 for success -1 for failures.
 */
int encrypt_decrypt_data(const pam_handle_t *pamh, int isencrypt,
			 const EVP_CIPHER *cipher, const char *key,
			 size_t keylen, const char *iv, size_t ivlen,
			 const char *inbytes, size_t inbyteslen, char *outbytes,
			 size_t *outbyteslen, char *mac, size_t *maclen)
{
	EVP_CIPHER_CTX *ctx;
	const EVP_MD *digest;
	size_t outEVPlen = 0;
	int retval = 0;
	size_t outlen = 0;

	if (cipher == NULL || key == NULL || iv == NULL || inbytes == NULL
	    || outbytes == NULL || mac == NULL || inbyteslen == 0
	    || EVP_CIPHER_key_length(cipher) > keylen
	    || EVP_CIPHER_iv_length(cipher) > ivlen) {
		pam_syslog(pamh, LOG_DEBUG, "Invalid inputs");
		return -1;
	}

	digest = EVP_sha256();
	if (!isencrypt) {
		char calmac[EVP_MAX_MD_SIZE];
		size_t calmaclen = 0;
		// calculate MAC for the encrypted message.
		if (NULL
		    == HMAC(digest, key, keylen, inbytes, inbyteslen, calmac,
			    &calmaclen)) {
			pam_syslog(pamh, LOG_DEBUG,
				   "Failed to verify authentication %d",
				   retval);
			return -1;
		}
		if (!((calmaclen == *maclen)
		      && (memcmp(calmac, mac, calmaclen) == 0))) {
			pam_syslog(pamh, LOG_DEBUG,
				   "Authenticated message doesn't match %d, %d",
				   calmaclen, *maclen);
			return -1;
		}
	}

	ctx = EVP_CIPHER_CTX_new();
	EVP_CIPHER_CTX_set_padding(ctx, 1);

	// Set key & IV
	retval = EVP_CipherInit_ex(ctx, cipher, NULL, key, iv, isencrypt);
	if (!retval) {
		pam_syslog(pamh, LOG_DEBUG, "EVP_CipherInit_ex failed with %d",
			   retval);
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	if ((retval = EVP_CipherUpdate(ctx, outbytes + outlen, &outEVPlen,
				       inbytes, inbyteslen))) {
		outlen += outEVPlen;
		if ((retval = EVP_CipherFinal(ctx, outbytes + outlen,
					      &outEVPlen))) {
			outlen += outEVPlen;
			*outbyteslen = outlen;
		} else {
			pam_syslog(pamh, LOG_DEBUG,
				   "EVP_CipherFinal returns with %d", retval);
			EVP_CIPHER_CTX_free(ctx);
			return -1;
		}
	} else {
		pam_syslog(pamh, LOG_DEBUG, "EVP_CipherUpdate returns with %d",
			   retval);
		EVP_CIPHER_CTX_free(ctx);
		return -1;
	}
	EVP_CIPHER_CTX_free(ctx);

	if (isencrypt) {
		// Create MAC for the encrypted message.
		if (NULL
		    == HMAC(digest, key, keylen, outbytes, *outbyteslen, mac,
			    maclen)) {
			pam_syslog(pamh, LOG_DEBUG,
				   "Failed to create authentication %d",
				   retval);
			return -1;
		}
	}
	return 0;
}


/**
 * @brief get temporary file handle
 * Function to get the temporary file handle, created using mkstemp
 *
 * @param[in] pamh - pam handle.
 * @param[in/out] tempfilename - tempfilename, which will be used in mkstemp.
 * @return - FILE handle for success. NULL for failure
 */
FILE *get_temp_file_handle(const pam_handle_t *pamh, char *const tempfilename)
{
	FILE *tempfile = NULL;
	int fd;
	int oldmask = umask(077);
	fd = mkstemp(tempfilename);
	if (fd == -1) {
		pam_syslog(pamh, LOG_DEBUG, "Error in creating temp file");
		umask(oldmask);
		return NULL;
	}
	pam_syslog(pamh, LOG_DEBUG, "Temporary file name is %s", tempfilename);

	tempfile = fdopen(fd, "w");
	umask(oldmask);
	return tempfile;
}


/**
 * @brief updates special password file
 * Function to update the special password file. Stores the password against
 * username in encrypted form along with meta data
 *
 * @param[in] pamh - pam handle.
 * @param[in] keyfilename - file name where key seed is stored.
 * @param[in] filename - special password file name
 * @param[in] forwho - name of the user
 * @param[in] towhat - password that has to stored in encrypted form
 * @return - PAM_SUCCESS for success or PAM_AUTHTOK_ERR for failure
 */
int update_pass_special_file(const pam_handle_t *pamh, const char *keyfilename,
			     const char *filename, const char *forwho,
			     const char *towhat)
{
	struct stat st;
	FILE *pwfile = NULL, *opwfile = NULL, *keyfile = NULL;
	int err = 0, wroteentry = 0;
	char tempfilename[1024];
	size_t forwholen = strlen(forwho);
	size_t towhatlen = strlen(towhat);
	char keybuff[MAX_KEY_SIZE] = {0};
	size_t keybuffsize = sizeof(keybuff);

	const EVP_CIPHER *cipher = EVP_aes_128_cbc();
	const EVP_MD *digest = EVP_sha256();

	char *linebuff = NULL, *opwfilebuff = NULL, *opwptext = NULL;
	size_t opwptextlen = 0, opwfilesize = 0;
	metapassstruct *opwmp = NULL;

	char *pwptext = NULL, *pwctext = NULL;
	size_t pwctextlen = 0, pwptextlen = 0, maclen = 0;
	size_t writtensize = 0, keylen = 0;
	metapassstruct pwmp = {META_PASSWD_SIG, {0, 0}, .0, 0, 0, 0, 0};
	char mac[EVP_MAX_MD_SIZE] = {0};
	unsigned char key[EVP_MAX_KEY_LENGTH];
	char iv[EVP_CIPHER_iv_length(cipher)];
	char hash[EVP_MD_block_size(digest)];

	// Following steps are performed in this function.
	// Step 1: Create a temporary file - always update temporary file, and
	// then swap it with original one, only if everything succeded at the
	// end. Step 2: If file already exists, read the old file and decrypt it
	// in buffer Step 3: Copy user/password pair from old buffer to new
	// buffer, and update, if the user already exists with the new password
	// Step 4: Encrypt the new buffer and write it to the temp file created
	// at Step 1.
	// Step 5. rename the temporary file name as special password file.

	// verify the tempfilename buffer is enough to hold
	// filename_XXXXXX (+1 for null).
	if (strlen(filename)
	    > (sizeof(tempfilename) - strlen("__XXXXXX") - 1)) {
		pam_syslog(pamh, LOG_DEBUG, "Not enough buffer, bailing out");
		return PAM_AUTHTOK_ERR;
	}
	// Fetch the key from key file name.
	keyfile = fopen(keyfilename, "r");
	if (keyfile == NULL) {
		pam_syslog(pamh, LOG_DEBUG, "Unable to open key file %s",
			   keyfilename);
		return PAM_AUTHTOK_ERR;
	}
	if (fread(keybuff, 1, keybuffsize, keyfile) != keybuffsize) {
		pam_syslog(pamh, LOG_DEBUG, "Key file read failed");
		fclose(keyfile);
		return PAM_AUTHTOK_ERR;
	}
	fclose(keyfile);

	// Step 1: Try to create a temporary file, in which all the update will
	// happen then it will be renamed to the original file. This is done to
	// have atomic operation.
	snprintf(tempfilename, sizeof(tempfilename), "%s__XXXXXX", filename);
	pwfile = get_temp_file_handle(pamh, tempfilename);
	if (pwfile == NULL) {
		err = 1;
		goto done;
	}

	// Update temporary file stat by reading the special password file
	opwfile = fopen(filename, "r");
	if (opwfile != NULL) {
		if (fstat(fileno(opwfile), &st) == -1) {
			fclose(opwfile);
			fclose(pwfile);
			err = 1;
			goto done;
		}
	} else { // Create with this settings if file is not present.
		memset(&st, 0, sizeof(st));
	}
	// Override the file permission with S_IWUSR | S_IRUSR
	st.st_mode = S_IWUSR | S_IRUSR;
	if ((fchown(fileno(pwfile), st.st_uid, st.st_gid) == -1)
	    || (fchmod(fileno(pwfile), st.st_mode) == -1)) {
		if (opwfile != NULL) {
			fclose(opwfile);
		}
		fclose(pwfile);
		err = 1;
		goto done;
	}
	opwfilesize = st.st_size;

	// Step 2: Read existing special password file and decrypt the data.
	if (opwfilesize) {
		opwfilebuff = malloc(opwfilesize);
		if (opwfilebuff == NULL) {
			fclose(opwfile);
			fclose(pwfile);
			err = 1;
			goto done;
		}

		if (fread(opwfilebuff, 1, opwfilesize, opwfile)) {
			opwmp = (metapassstruct *)opwfilebuff;
			opwptext = malloc(opwmp->datasize + opwmp->padsize);
			if (opwptext == NULL) {
				free(opwfilebuff);
				fclose(opwfile);
				fclose(pwfile);
				err = 1;
				goto done;
			}
			// User & password pairs are mapped as <user
			// name>:<password>\n. Add +3 for special chars ':',
			// '\n' and '\0'.
			pwptextlen = opwmp->datasize + forwholen + towhatlen + 3
				     + EVP_CIPHER_block_size(cipher);
			pwptext = malloc(pwptextlen);
			if (pwptext == NULL) {
				free(opwptext);
				free(opwfilebuff);
				fclose(opwfile);
				fclose(pwfile);
				err = 1;
				goto done;
			}

			// First get the hashed key to decrypt
			HMAC(digest, keybuff, keybuffsize,
			     opwfilebuff + sizeof(*opwmp), opwmp->hashsize, key,
			     &keylen);

			// Skip decryption if there is no data
			if (opwmp->datasize != 0) {
				// Do the decryption
				if (encrypt_decrypt_data(
					    pamh, 0, cipher, key, keylen,
					    opwfilebuff + sizeof(*opwmp)
						    + opwmp->hashsize,
					    opwmp->ivsize,
					    opwfilebuff + sizeof(*opwmp)
						    + opwmp->hashsize
						    + opwmp->ivsize,
					    opwmp->datasize + opwmp->padsize,
					    opwptext, &opwptextlen,
					    opwfilebuff + sizeof(*opwmp)
						    + opwmp->hashsize
						    + opwmp->ivsize
						    + opwmp->datasize
						    + opwmp->padsize,
					    &opwmp->macsize)
				    != 0) {
					pam_syslog(pamh, LOG_DEBUG,
						   "Decryption failed");
					free(pwptext);
					free(opwptext);
					free(opwfilebuff);
					fclose(opwfile);
					fclose(pwfile);
					err = 1;
					goto done;
				}
			}

			// NULL terminate it, before using it in strtok().
			opwptext[opwmp->datasize] = '\0';

			linebuff = strtok(opwptext, "\n");
			// Step 3: Copy the existing user/password pair
			// to the new buffer, and update the password if user
			// already exists.
			while (linebuff != NULL) {
				if ((!strncmp(linebuff, forwho, forwholen))
				    && (linebuff[forwholen] == ':')) {
					writtensize += snprintf(
						pwptext + writtensize,
						pwptextlen - writtensize,
						"%s:%s\n", forwho, towhat);
					wroteentry = 1;
				} else {
					writtensize += snprintf(
						pwptext + writtensize,
						pwptextlen - writtensize,
						"%s\n", linebuff);
				}
				linebuff = strtok(NULL, "\n");
			}
		}
		// Clear the old password related buffers here, as we are done
		// with it.
		free(opwfilebuff);
		free(opwptext);
	} else {
		pwptextlen = forwholen + towhatlen + 3
			     + EVP_CIPHER_block_size(cipher);
		pwptext = malloc(pwptextlen);
		if (pwptext == NULL) {
			if (opwfile != NULL) {
				fclose(opwfile);
			}
			fclose(pwfile);
			err = 1;
			goto done;
		}
	}

	if (opwfile != NULL) {
		fclose(opwfile);
	}

	if (!wroteentry) {
		// Write the new user:password pair at the end.
		writtensize += snprintf(pwptext + writtensize,
					pwptextlen - writtensize, "%s:%s\n",
					forwho, towhat);
	}
	pwptextlen = writtensize;

	// Step 4: Encrypt the data and write to the temporary file
	if (RAND_bytes(hash, EVP_MD_block_size(digest)) != 1) {
		pam_syslog(pamh, LOG_DEBUG,
			   "Hash genertion failed, bailing out");
		free(pwptext);
		fclose(pwfile);
		err = 1;
		goto done;
	}

	// Generate hash key, which will be used for encryption.
	HMAC(digest, keybuff, keybuffsize, hash, EVP_MD_block_size(digest), key,
	     &keylen);
	// Generate IV values
	if (RAND_bytes(iv, EVP_CIPHER_iv_length(cipher)) != 1) {
		pam_syslog(pamh, LOG_DEBUG,
			   "IV generation failed, bailing out");
		free(pwptext);
		fclose(pwfile);
		err = 1;
		goto done;
	}

	// Buffer to store encrypted message.
	pwctext = malloc(pwptextlen + EVP_CIPHER_block_size(cipher));
	if (pwctext == NULL) {
		pam_syslog(pamh, LOG_DEBUG, "Ctext buffer failed, bailing out");
		free(pwptext);
		fclose(pwfile);
		err = 1;
		goto done;
	}

	// Do the encryption
	if (encrypt_decrypt_data(pamh, 1, cipher, key, keylen, iv,
				 EVP_CIPHER_iv_length(cipher), pwptext,
				 pwptextlen, pwctext, &pwctextlen, mac, &maclen)
	    != 0) {
		pam_syslog(pamh, LOG_DEBUG, "Encryption failed");
		free(pwctext);
		free(pwptext);
		fclose(pwfile);
		err = 1;
		goto done;
	}

	// Update the meta password structure.
	pwmp.hashsize = EVP_MD_block_size(digest);
	pwmp.ivsize = EVP_CIPHER_iv_length(cipher);
	pwmp.datasize = writtensize;
	pwmp.padsize = pwctextlen - writtensize;
	pwmp.macsize = maclen;

	// Write the meta password structure, followed by hash, iv, encrypted
	// data & mac.
	if (fwrite(&pwmp, 1, sizeof(pwmp), pwfile) != sizeof(pwmp)) {
		pam_syslog(pamh, LOG_DEBUG, "Error in writing meta data");
		err = 1;
	}
	if (fwrite(hash, 1, pwmp.hashsize, pwfile) != pwmp.hashsize) {
		pam_syslog(pamh, LOG_DEBUG, "Error in writing hash data");
		err = 1;
	}
	if (fwrite(iv, 1, pwmp.ivsize, pwfile) != pwmp.ivsize) {
		pam_syslog(pamh, LOG_DEBUG, "Error in writing IV data");
		err = 1;
	}
	if (fwrite(pwctext, 1, pwctextlen, pwfile) != pwctextlen) {
		pam_syslog(pamh, LOG_DEBUG, "Error in encrypted data");
		err = 1;
	}
	if (fwrite(mac, 1, maclen, pwfile) != maclen) {
		pam_syslog(pamh, LOG_DEBUG, "Error in writing MAC");
		err = 1;
	}

	free(pwctext);
	free(pwptext);

	if (fflush(pwfile) || fsync(fileno(pwfile))) {
		pam_syslog(
			pamh, LOG_DEBUG,
			"fflush or fsync error writing entries to special file: %s",
			tempfilename);
		err = 1;
	}

	if (fclose(pwfile)) {
		pam_syslog(pamh, LOG_DEBUG,
			   "fclose error writing entries to special file: %s",
			   tempfilename);
		err = 1;
	}

done:
	if (!err) {
		// Step 5: Rename the temporary file as special password file.
		if (!rename(tempfilename, filename)) {
			pam_syslog(pamh, LOG_DEBUG,
				   "password changed for %s in special file",
				   forwho);
		} else {
			err = 1;
		}
	}

	// Clear out the key buff.
	memset(keybuff, 0, keybuffsize);

	if (!err) {
		return PAM_SUCCESS;
	} else {
		unlink(tempfilename);
		return PAM_AUTHTOK_ERR;
	}
}


/* Password Management API's */

/**
 * @brief pam_sm_chauthtok API
 * Function which will be called for pam_chauthtok() calls.
 *
 * @param[in] pamh - pam handle
 * @param[in] flags - pam calls related flags
 * @param[in] argc - argument counts / options
 * @param[in] argv - array of arguments / options
 * @return - PAM_SUCCESS for success, others for failure
 */
int pam_sm_chauthtok(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
	int retval = -1;
	const void *item = NULL;
	const char *user = NULL;
	const char *pass_new = NULL, *pass_old = NULL;
	const char *spec_grp_name =
		get_option(pamh, "spec_grp_name", argc, argv);
	const char *spec_pass_file =
		get_option(pamh, "spec_pass_file", argc, argv);
	const char *key_file = get_option(pamh, "key_file", argc, argv);


	if (spec_grp_name == NULL || key_file == NULL) {
		return PAM_IGNORE;
	}
	if (flags & PAM_PRELIM_CHECK) {
		// send success to verify other stacked modules prelim check.
		return PAM_SUCCESS;
	}

	retval = pam_get_user(pamh, &user, NULL);
	if (retval != PAM_SUCCESS) {
		return retval;
	}

	// get  already read password by the stacked pam module
	// Note: If there are no previous stacked pam module which read
	// the new password, then return with AUTHTOK_ERR

	retval = pam_get_item(pamh, PAM_AUTHTOK, &item);
	if (retval != PAM_SUCCESS || item == NULL) {
		return PAM_AUTHTOK_ERR;
	}
	pass_new = item;

	struct group *grp;
	int spec_grp_usr = 0;
	// Verify whether the user belongs to special group.
	grp = pam_modutil_getgrnam(pamh, spec_grp_name);
	if (grp != NULL) {
		while (*(grp->gr_mem) != NULL) {
			if (strcmp(user, *grp->gr_mem) == 0) {
				spec_grp_usr = 1;
				break;
			}
			(grp->gr_mem)++;
		}
	}

	pam_syslog(pamh, LOG_DEBUG, "User belongs to special grp: %x",
		   spec_grp_usr);

	if (spec_grp_usr) {
		// verify the new password is acceptable.
		if (strlen(pass_new) > MAX_SPEC_GRP_PASS_LENGTH
		    || strlen(user) > MAX_SPEC_GRP_USER_LENGTH) {
			pam_syslog(
				pamh, LOG_ERR,
				"Password length (%x) / User name length (%x) not acceptable",
				strlen(pass_new), strlen(user));
			pass_new = NULL;
			return PAM_NEW_AUTHTOK_REQD;
		}
		if (spec_pass_file == NULL) {
			spec_pass_file = DEFAULT_SPEC_PASS_FILE;
			pam_syslog(
				pamh, LOG_ERR,
				"Using default special password file name :%s",
				spec_pass_file);
		}
		if (retval = lock_pwdf()) {
			pam_syslog(pamh, LOG_ERR,
				   "Failed to lock the passwd file");
			return retval;
		}
		retval = update_pass_special_file(
			pamh, key_file, spec_pass_file, user, pass_new);
		unlock_pwdf();
		return retval;
	}

	return PAM_SUCCESS;
}

/* end of module definition */
