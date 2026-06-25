/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#define _POSIX_C_SOURCE 200809L

#include "enser/decoder.h"
#include "enser/encoder.h"

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

int encr_decoder(
    const uint8_t *buffer,
    size_t buffer_size,
    encr_object_t *out
){
    if (!buffer || !out)
    return ENCR_ERR_CORRUPTED;

    memset(out, 0, sizeof(*out));

    if (buffer_size < sizeof(encr_header_t))
        return ENCR_ERR_SIZE;

    encr_header_t hdr;

    memcpy(&hdr, buffer, sizeof(hdr));

    if (memcmp(hdr.magic, "ENCR", 4) != 0)
        return ENCR_ERR_MAGIC;

    if (hdr.version != ENCR_VERSION)
        return ENCR_ERR_VERSION;

    uint16_t refs_count =
        ntohs(hdr.refs_count);

    uint32_t payload_size =
        ntohl(hdr.payload_size);

    uint64_t causal_seq =
      enser_be64toh(hdr.causal_seq);

    (void)causal_seq;

    size_t expected = encr_size
        (
            refs_count,
            payload_size
        );

    if (expected != buffer_size){
        return ENCR_ERR_CORRUPTED;
    }

    const uint8_t *ptr = buffer + sizeof(encr_header_t) + (refs_count * ENCR_HASH_SIZE);


    out->payload = malloc(payload_size);

    if (payload_size > 0 &&
        !out->payload)
        return ENCR_ERR_OVERFLOW;

    memcpy(
        out->payload,
        ptr,
        payload_size
    );

    memcpy(
        out->header.magic,
        hdr.magic,
        4
    );

    out->header.version = hdr.version;
    out->header.refs_count = refs_count;
    out->header.payload_size = payload_size;
    out->header.causal_seq = causal_seq;

    memcpy(
        out->header.hash,
        hdr.hash,
        32
    );

    out->header.refs_count = refs_count;
    out->header.payload_size = payload_size;

    return ENCR_OK;
}

void encr_object_free(
    encr_object_t *obj
){
    if (!obj)
        return;

    free(obj->payload);

    obj->payload = NULL;
}
