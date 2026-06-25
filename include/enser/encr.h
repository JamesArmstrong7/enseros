/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSER_ENCR_H
#define ENSER_ENCR_H

#include <stdint.h>
#include <stddef.h>

#define ENCR_MAGIC "ENCR"

#define ENCR_VERSION 1

#define ENCR_HASH_SIZE 32

#include <arpa/inet.h>

static inline uint64_t enser_htobe64(uint64_t host)
{
    return ((uint64_t)htonl(host & 0xFFFFFFFFULL) << 32)
         | htonl(host >> 32);
}

static inline uint64_t enser_be64toh(uint64_t net)
{
    return ((uint64_t)ntohl(net & 0xFFFFFFFFULL) << 32)
         | ntohl(net >> 32);
}

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

    uint64_t causal_seq;

    uint8_t hash[ENCR_HASH_SIZE];

} encr_header_t;

#pragma pack(pop)

#endif