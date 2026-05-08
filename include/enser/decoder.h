#ifndef ENSER_DECODER_H
#define ENSER_DECODER_H

#include <stdint.h>
#include <stddef.h>

#include "enser/encr.h"

typedef struct {

    encr_header_t header;

    uint8_t **refs;

    uint8_t *payload;

} encr_object_t;

int encr_decoder(
    const uint8_t *buffer,
    size_t buffer_size,
    encr_object_t *out
);

void encr_object_free(
    encr_object_t *obj
);

#endif
