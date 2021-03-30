/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (C) 2018 IBM Corp. */
#pragma once

#include <stdint.h>
#include <sys/types.h>

/* There are two structures outlined here - one that represents the PNOR
 * partition table (or header) - this appears first in the PNOR image.
 * The last field of the PNOR partition table structure is an array
 * of another structure - which represents the partition.
 *
 * The flash structures used here have been borrowed from
 * https://github.com/open-power/hostboot/blob/master/src/usr/pnor/ffs.h */


/* The maximum length of a partition's name */
#define PARTITION_NAME_MAX 15

/* The version of this partition implementation. This is an
 * incrementing value */
#define PARTITION_VERSION_1 1

/* Magic number for the partition partition_table (ASCII 'PART') */
#define PARTITION_HEADER_MAGIC 0x50415254

/* Default parent partition id */
#define PARENT_PATITION_ID 0xFFFFFFFF

/* The partition structure has 16 'user data' words, which can be used to store
 * miscellaneous information. This is typically used to store bits that state
 * whether a partition is ECC protected, is read-only, is preserved across
 * updates, etc.
 *
 * TODO: Replace with libflash (!) or at least refactor the data structures to
 * better match hostboot's layout[1]. The latter would avoid the headache of
 * verifying these flags match the expected functionality (taking into account
 * changes in endianness).
 *
 * [1] https://github.com/open-power/hostboot/blob/9acfce99596f12dcc60952f8506a77e542609cbf/src/usr/pnor/common/ffs_hb.H#L81
 */
#define PARTITION_USER_WORDS 16
#define PARTITION_ECC_PROTECTED 0x8000
#define PARTITION_PRESERVED 0x00800000
#define PARTITION_READONLY 0x00400000
#define PARTITION_REPROVISION 0x00100000
#define PARTITION_VOLATILE 0x00080000
#define PARTITION_CLEARECC 0x00040000
#define PARTITION_VERSION_CHECK_SHA512 0x80000000
#define PARTITION_VERSION_CHECK_SHA512_PER_EC 0x40000000

/* Partition flags */
enum partition_flags {
    PARTITION_FLAGS_PROTECTED = 0x0001,
    PARTITION_FLAGS_U_BOOT_ENV = 0x0002
};

/* Type of image contained within partition */
enum partition_type {
    PARTITION_TYPE_DATA = 1,
    PARTITION_TYPE_LOGICAL = 2,
    PARTITION_TYPE_PARTITION = 3
};


/**
 * struct pnor_partition
 *
 * @name:       Name of the partition - a null terminated string
 * @base:       The offset in the PNOR, in block-size (1 block = 4KB),
 *              where this partition is placed
 * @size:       Partition size in blocks.
 * @pid:        Parent partition id
 * @id:         Partition ID [1..65536]
 * @type:       Type of partition, see the 'type' enum
 * @flags:      Partition flags (optional), see the 'flags' enum
 * @actual:     Actual partition size (in bytes)
 * @resvd:      Reserved words for future use
 * @user:       User data (optional), see user data macros above
 * @checksum:   Partition checksum (includes all words above) - the
 *              checksum is obtained by a XOR operation on all of the
 *              words above. This is used for detecting a corruption
 *              in this structure
 */
struct pnor_partition {
    struct {
        char         name[PARTITION_NAME_MAX + 1];
        uint32_t     base;
        uint32_t     size;
        uint32_t     pid;
        uint32_t     id;
        uint32_t     type;
        uint32_t     flags;
        uint32_t     actual;
        uint32_t     resvd[4];
        struct
        {
            uint32_t data[PARTITION_USER_WORDS];
        } user;
    } __attribute__ ((packed)) data;
    uint32_t     checksum;
} __attribute__ ((packed));

/**
 * struct pnor_partition_table
 *
 * @magic:          Eye catcher/corruption detector - set to
 *                  PARTITION_HEADER_MAGIC
 * @version:        Version of the structure, set to
 *                  PARTITION_VERSION_1
 * @size:           Size of partition table (in blocks)
 * @entry_size:     Size of struct pnor_partition element (in bytes)
 * @entry_count:    Number of struct pnor_partition elements in partitions array
 * @block_size:     Size of an erase-block on the PNOR (in bytes)
 * @block_count:    Number of blocks on the PNOR
 * @resvd:          Reserved words for future use
 * @checksum:       Header checksum (includes all words above) - the
 *                  checksum is obtained by a XOR operation on all of the
 *                  words above. This is used for detecting a corruption
 *                  in this structure
 * @partitions:     Array of struct pnor_partition
 */
struct pnor_partition_table {
    struct {
        uint32_t         magic;
        uint32_t         version;
        uint32_t         size;
        uint32_t         entry_size;
        uint32_t         entry_count;
        uint32_t         block_size;
        uint32_t         block_count;
        uint32_t         resvd[4];
    } __attribute__ ((packed)) data;
    uint32_t         checksum;
    struct pnor_partition partitions[];
} __attribute__ ((packed));
