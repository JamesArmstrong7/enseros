/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSER_HASH_H
#define ENSER_HASH_H

#include <stdint.h>
#include <stddef.h>

#define HASH_SIZE 32

int hash_sha256(
    const uint8_t *data,
    size_t len,
    uint8_t out[HASH_SIZE]
);

#endif
