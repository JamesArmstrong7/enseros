/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include <stdio.h>
#include <stdlib.h>

#include "enser/decoder.h"
#include "enser/encoder.h"

int test_parse(void)
{
    uint8_t *buffer = NULL;

    size_t size = 0;

    const char payload[] =
        "ENSER PARSE TEST";

    int result =
        encr_build(
            NULL,
            0,
            (const uint8_t *)payload,
            sizeof(payload) - 1,
            &buffer,
            &size
        );

    if (result != ENCR_OK){
        fprintf(
            stderr,
            "build failed\n"
        );

        return 1;
    }

    encr_object_t obj;

    result =
        encr_decoder(
            buffer,
            size,
            &obj
        );

    free(buffer);

    if (result != ENCR_OK){
        fprintf(
            stderr,
            "decode failed: %d\n",
            result
        );

        return 1;
    }

    printf(
        "parse ok\n"
    );

    printf(
        "refs_count: %u\n",
        obj.header.refs_count
    );

    printf(
        "payload_size: %u\n",
        obj.header.payload_size
    );

    printf(
        "payload: %.*s\n",
        (int)obj.header.payload_size,
        obj.payload
    );

    encr_object_free(&obj);

    return 0;
}
