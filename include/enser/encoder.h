#ifndef ENSER_ENCODER_H
#define ENSER_ENCODER_H

#include <stdint.h>
#include <stddef.h>

#include "enser/encr.h"

size_t encr_size(
    uint16_t refs_count,
    uint32_t payload_size
);

int encr_build(
    const uint8_t **refs,
    uint16_t refs_count,
    const uint8_t *payload,
    uint32_t payload_size,
    uint8_t **out_buffer,
    size_t *out_size
);

int encr_validate(
    const uint8_t *buffer,
    size_t size
);

int encr_hash(
    const uint8_t *buffer,
    size_t size,
    uint8_t out[ENCR_HASH_SIZE]
);

#endif