/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "enser/encoder.h"
#include "enser/storage.h"
#include "enser/hex.h"

int test_create(void) {
    uint8_t *buffer = NULL;
    size_t size = 0;

    uint8_t hash[32];

    // Provisional!
    //
    const char payload[] =
    "hello enser";
    // 
    // OK!

    if (
        encr_build(
            NULL,
            0,
            (const uint8_t *)payload,
            sizeof(payload) - 1,
            &buffer,
            &size
        ) != 0
    ) {
        printf("encoder_build failed\n");
        return 1;
    }

    printf("Event size: %zu bytes\n", size);

    if (
        encr_validate(
            buffer,
            size
        ) != 0
    ) {
        printf("encoder_validate failed\n");
        free(buffer);
        return 1;
    }

    if (
        encr_hash(
            buffer,
            size,
            hash
        ) != 0
    ) {
        printf("encoder_hash failed\n");
        free(buffer);
        return 1;
    }

    char hash_hex[65];

    for (int i = 0; i < 32; i++) {
        sprintf(
            &hash_hex[i * 2],
            "%02x",
            hash[i]
        );
    }

    hash_hex[64] = '\0';

    if (
        ensor_storage_write(
            hash_hex,
            buffer,
            size
        ) != 0
    ) {
        printf("storage write failed\n");
        free(buffer);
        return 1;
    }

    printf("Stored event: %s\n", hash_hex);

    free(buffer);

    return 0;
}
