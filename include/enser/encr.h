#ifndef ENSER_ENCR_H
#define ENSER_ENCR_H

#include <stdint.h>
#include <stddef.h>

#define ENCR_MAGIC "ENCR"

#define ENCR_VERSION 1

#define ENCR_HASH_SIZE 32

typedef enum {

    ENCR_OK = 0,

    ENCR_ERR_MAGIC = -1,
    ENCR_ERR_VERSION = -2,
    ENCR_ERR_SIZE = -3,
    ENCR_ERR_CORRUPTED = -4,
    ENCR_ERR_OVERFLOW = -5

} encr_result_t;

#pragma pack(push, 1)

typedef struct {

    char magic[4];

    uint8_t version;

    uint16_t refs_count;
    uint32_t payload_size;

    uint64_t timestamp;

    uint8_t hash[ENCR_HASH_SIZE];

} encr_header_t;

#pragma pack(pop)

#endif
