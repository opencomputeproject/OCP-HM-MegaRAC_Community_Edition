#pragma once

#include "manager.hpp"

#include <ipmid/api.h>

#include <blobs-ipmid/blobs.hpp>
#include <string>

namespace blobs
{

/* Used by bmcBlobGetCount */
struct BmcBlobCountTx
{
    uint8_t cmd; /* bmcBlobGetCount */
} __attribute__((packed));

struct BmcBlobCountRx
{
    uint16_t crc;
    uint32_t blobCount;
} __attribute__((packed));

/* Used by bmcBlobEnumerate */
struct BmcBlobEnumerateTx
{
    uint8_t cmd; /* bmcBlobEnumerate */
    uint16_t crc;
    uint32_t blobIdx;
} __attribute__((packed));

struct BmcBlobEnumerateRx
{
    uint16_t crc;
    char blobId[];
} __attribute__((packed));

/* Used by bmcBlobOpen */
struct BmcBlobOpenTx
{
    uint8_t cmd; /* bmcBlobOpen */
    uint16_t crc;
    uint16_t flags;
    char blobId[]; /* Must correspond to a valid blob. */
} __attribute__((packed));

struct BmcBlobOpenRx
{
    uint16_t crc;
    uint16_t sessionId;
} __attribute__((packed));

/* Used by bmcBlobClose */
struct BmcBlobCloseTx
{
    uint8_t cmd; /* bmcBlobClose */
    uint16_t crc;
    uint16_t sessionId; /* Returned from BmcBlobOpen. */
} __attribute__((packed));

/* Used by bmcBlobDelete */
struct BmcBlobDeleteTx
{
    uint8_t cmd; /* bmcBlobDelete */
    uint16_t crc;
    char blobId[];
} __attribute__((packed));

/* Used by bmcBlobStat */
struct BmcBlobStatTx
{
    uint8_t cmd; /* bmcBlobStat */
    uint16_t crc;
    char blobId[];
} __attribute__((packed));

struct BmcBlobStatRx
{
    uint16_t crc;
    uint16_t blobState;
    uint32_t size; /* Size in bytes of the blob. */
    uint8_t metadataLen;
    uint8_t metadata[]; /* Optional blob-specific metadata. */
} __attribute__((packed));

/* Used by bmcBlobSessionStat */
struct BmcBlobSessionStatTx
{
    uint8_t cmd; /* bmcBlobSessionStat */
    uint16_t crc;
    uint16_t sessionId;
} __attribute__((packed));

/* Used by bmcBlobCommit */
struct BmcBlobCommitTx
{
    uint8_t cmd; /* bmcBlobCommit */
    uint16_t crc;
    uint16_t sessionId;
    uint8_t commitDataLen;
    uint8_t commitData[]; /* Optional blob-specific commit data. */
} __attribute__((packed));

/* Used by bmcBlobRead */
struct BmcBlobReadTx
{
    uint8_t cmd; /* bmcBlobRead */
    uint16_t crc;
    uint16_t sessionId;
    uint32_t offset;        /* The byte sequence start, 0-based. */
    uint32_t requestedSize; /* The number of bytes requested for reading. */
} __attribute__((packed));

struct BmcBlobReadRx
{
    uint16_t crc;
    uint8_t data[];
} __attribute__((packed));

/* Used by bmcBlobWrite */
struct BmcBlobWriteTx
{
    uint8_t cmd; /* bmcBlobWrite */
    uint16_t crc;
    uint16_t sessionId;
    uint32_t offset; /* The byte sequence start, 0-based. */
    uint8_t data[];
} __attribute__((packed));

/* Used by bmcBlobWriteMeta */
struct BmcBlobWriteMetaTx
{
    uint8_t cmd; /* bmcBlobWriteMeta */
    uint16_t crc;
    uint16_t sessionId; /* Returned from BmcBlobOpen. */
    uint32_t offset;    /* The byte sequence start, 0-based. */
    uint8_t data[];
} __attribute__((packed));

/**
 * Validate the minimum request length if there is one.
 *
 * @param[in] subcommand - the command
 * @param[in] requestLength - the length of the request
 * @return bool - true if valid.
 */
bool validateRequestLength(BlobOEMCommands command, size_t requestLen);

/**
 * Given a pointer into an IPMI request buffer and the length of the remaining
 * buffer, builds a string.  This does no string validation w.r.t content.
 *
 * @param[in] start - the start of the expected string.
 * @param[in] length - the number of bytes remaining in the buffer.
 * @return the string if valid otherwise an empty string.
 */
std::string stringFromBuffer(const char* start, size_t length);

/**
 * Writes out a BmcBlobCountRx structure and returns IPMI_OK.
 */
ipmi_ret_t getBlobCount(ManagerInterface* mgr, const uint8_t* reqBuf,
                        uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Writes out a BmcBlobEnumerateRx in response to a BmcBlobEnumerateTx
 * request.  If the index does not correspond to a blob, then this will
 * return failure.
 *
 * It will also return failure if the response buffer is of an invalid
 * length.
 */
ipmi_ret_t enumerateBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                         uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempts to open the blobId specified and associate with a session id.
 */
ipmi_ret_t openBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                    uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempts to close the session specified.
 */
ipmi_ret_t closeBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                     uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempts to delete the blobId specified.
 */
ipmi_ret_t deleteBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                      uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempts to retrieve the Stat for the blobId specified.
 */
ipmi_ret_t statBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                    uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempts to retrieve the Stat for the session specified.
 */
ipmi_ret_t sessionStatBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                           uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempts to commit the data in the blob.
 */
ipmi_ret_t commitBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                      uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempt to read data from the blob.
 */
ipmi_ret_t readBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                    uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempt to write data to the blob.
 */
ipmi_ret_t writeBlob(ManagerInterface* mgr, const uint8_t* reqBuf,
                     uint8_t* replyCmdBuf, size_t* dataLen);

/**
 * Attempt to write metadata to the blob.
 */
ipmi_ret_t writeMeta(ManagerInterface* mgr, const uint8_t* reqBuf,
                     uint8_t* replyCmdBuf, size_t* dataLen);

} // namespace blobs
