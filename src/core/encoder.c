#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "enser/encoder.h"
#include "enser/hash.h"

size_t encoder_size(uint16_t refs_count, uint32_t payload_size) {
    return sizeof(encoder_header_t)
        + (size_t)refs_count * ENCODER_HASH_SIZE
        + payload_size;
}

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
) {
    if (!out_buffer || !out_size) return -1;
    if (refs_count > 0 && !refs) return -2;

    size_t total = encoder_size(refs_count, payload_size);

    uint8_t *buf = malloc(total);
    if (!buf) return -3;

    encoder_header_t *hdr = (encoder_header_t *)buf;

    memcpy(hdr->magic, ENCODER_MAGIC, 4);
    hdr->version = ENCODER_VERSION;

    hdr->sys = sys;
    hdr->id  = id;
    hdr->ix  = ix;
    hdr->ec  = ec;

    hdr->refs_count = htons(refs_count);
    hdr->payload_size = htonl(payload_size);

    uint8_t *ptr = buf + sizeof(encoder_header_t);

    // refs
    for (uint16_t i = 0; i < refs_count; i++) {
        memcpy(ptr, refs[i], ENCODER_HASH_SIZE);
        ptr += ENCODER_HASH_SIZE;
    }

    // payload
    if (payload_size > 0 && payload) {
        memcpy(ptr, payload, payload_size);
    }

    *out_buffer = buf;
    *out_size = total;

    return 0;
}

int encoder_validate(const uint8_t *buffer, size_t size) {
    if (!buffer || size < sizeof(encoder_header_t)) return -1;

    const encoder_header_t *hdr = (const encoder_header_t *)buffer;

    if (memcmp(hdr->magic, ENCODER_MAGIC, 4) != 0) return -2;
    if (hdr->version != ENCODER_VERSION) return -3;

    size_t expected = encoder_size(hdr->refs_count, hdr->payload_size);

    if (expected != size) return -4;

    return 0;
}

int encoder_hash(
    const uint8_t *buffer,
    size_t size,
    uint8_t out[ENCODER_HASH_SIZE]
) {
    if (!buffer || !out) return -1;
    return hash_sha256(buffer, size, out);
}

int encoder_parse(
    const uint8_t *buffer,
    size_t size,
    encoder_view_t *out
) {
    if (!buffer || !out) return -1;

    // reutiliza tu validate
    if (encoder_validate(buffer, size) != 0) return -2;

    const encoder_header_t *hdr = (const encoder_header_t *)buffer;

    // ⚠️ si aún no aplicas hton/ntoh, usa directo
    uint16_t refs_count = hdr->refs_count;
    uint32_t payload_size = hdr->payload_size;

    const uint8_t *ptr = buffer + sizeof(encoder_header_t);

    const uint8_t *refs = ptr;
    ptr += (size_t)refs_count * ENCODER_HASH_SIZE;

    const uint8_t *payload = ptr;

    // llenar vista
    out->hdr = hdr;
    out->refs = refs;
    out->payload = payload;
    out->refs_count = refs_count;
    out->payload_size = payload_size;

    return 0;
}
