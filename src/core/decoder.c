#define _POSIX_C_SOURCE 200809L

#include "enser/decoder.h"
#include "enser/encoder.h"

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>

int encr_decoder(
    const uint8_t *buffer,
    size_t buffer_size,
    encr_object_t *out
){
    memset(out, 0, sizeof(*out));

    if (buffer_size < sizeof(encr_header_t))
        return ENCR_ERR_SIZE;

    encr_header_t hdr;

    memcpy(&hdr, buffer, sizeof(hdr));

    if (memcmp(hdr.magic, "ENCR", 4) != 0)
        return ENCR_ERR_MAGIC;

    if (hdr.version != ENCODER_VERSION)
        return ENCR_ERR_VERSION;

    uint16_t refs_count =
        ntohs(hdr.refs_count);

    uint32_t payload_size =
        ntohl(hdr.payload_size);

    uint64_t timestamp =
        hdr.timestamp;

    (void)timestamp;

    size_t expected =
        sizeof(encr_header_t) + payload_size;

    if (expected != buffer_size)
        return ENCR_ERR_CORRUPTED;

    const uint8_t *ptr =
        buffer + sizeof(encr_header_t);

    out->payload = malloc(payload_size);

    if (!out->payload)
        return ENCR_ERR_OVERFLOW;

    memcpy(
        out->payload,
        ptr,
        payload_size
    );

    out->header = hdr;

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
