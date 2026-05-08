#ifndef ENSER_HEX_H
#define ENSER_HEX_H

#include <stdint.h>

void hash_to_hex(
    const uint8_t hash[32],
    char out[65]
);

int hex_to_hash(
    const char *hex,
    uint8_t out[32]
);

#endif
