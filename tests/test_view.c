#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "enser/decoder.h"

static uint8_t *read_file(
    const char *path,
    size_t *size_out
){
    FILE *fp = fopen(path, "rb");

    if (!fp)
        return NULL;

    if (fseek(fp, 0, SEEK_END) != 0){
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);

    if (size < 0){
        fclose(fp);
        return NULL;
    }

    rewind(fp);

    uint8_t *buffer =
        malloc((size_t)size);

    if (!buffer){
        fclose(fp);
        return NULL;
    }

    size_t read_bytes =
        fread(
            buffer,
            1,
            (size_t)size,
            fp
        );

    fclose(fp);

    if (read_bytes != (size_t)size){
        free(buffer);
        return NULL;
    }

    *size_out = (size_t)size;

    return buffer;
}

int test_view(
    const char *path
){
    size_t file_size = 0;

    uint8_t *file_buffer =
        read_file(path, &file_size);

    if (!file_buffer){
        fprintf(
            stderr,
            "failed to read file\n"
        );

        return 1;
    }

    encr_object_t obj;

    int result =
        encr_decoder(
            file_buffer,
            file_size,
            &obj
        );

    free(file_buffer);

    if (result != ENCR_OK){
        fprintf(
            stderr,
            "decoder error: %d\n",
            result
        );

        return 1;
    }

    printf(
        "\n=== ENCODING OBJECT ===\n\n"
    );

    printf(
        "magic: %.4s\n",
        obj.header.magic
    );

    printf(
        "version: %u\n",
        obj.header.version
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
        "timestamp: %llu\n",
        (unsigned long long)
        obj.header.causal_seq
    );

    printf("\nhash: ");

    for (int i = 0; i < 32; i++){
        printf(
            "%02x",
            obj.header.hash[i]
        );
    }

    printf("\n");

    printf("\npayload:\n\n");

    fwrite(
        obj.payload,
        1,
        obj.header.payload_size,
        stdout
    );

    printf("\n");

    encr_object_free(&obj);

    return 0;
}
