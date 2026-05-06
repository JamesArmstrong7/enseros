#ifndef ENSER_ENCODER_H
#define ENSER_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include "hash.h"

#define ENCODER_MAGIC "ENCR"
#define ENCODER_VERSION 1
#define ENCODER_HASH_SIZE 32

#pragma pack(push, 1)
typedef struct {
    uint8_t magic[4];
    uint8_t version;

    uint8_t sys;    // ENSER GLOBAL SYSTEM EVENT
    uint8_t id;     // GENERAL IDENTIFIER EVENT
    uint8_t ix;     // INDEX CONTEXT SOURCE EVENT
    uint8_t ec;     // CONTRACT AND CONTROL EVENT 

    uint16_t refs_count;
    uint32_t payload_size;
} encoder_header_t;
#pragma pack(pop)

typedef struct {
    const encoder_header_t *hdr;

    const uint8_t *refs;     // bloque continuo (refs_count * 32)
    const uint8_t *payload;

    uint16_t refs_count;
    uint32_t payload_size;
} encoder_view_t;

size_t encoder_size(
    uint16_t refs_count,
    uint32_t payload_size
);

int encoder_build(
    uint8_t sys,
    uint8_t id,
    uint8_t ix,
    uint8_t ec,
    const uint8_t **refs,
    uint16_t refs_count,
    const uint8_t *payload,
    uint32_t payload_size,
    uint8_t **out_buffer,
    size_t *out_size
);

int encoder_validate(
    const uint8_t *buffer,
    size_t size
);

int encoder_hash(
    const uint8_t *buffer,
    size_t size,
    uint8_t out[ENCODER_HASH_SIZE]
);

int encoder_parse(
    const uint8_t *buffer,
    size_t size,
    encoder_view_t *out
);

#endif
