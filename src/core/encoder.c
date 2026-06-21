#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>

#include "enser/encoder.h"
#include "enser/hash.h"
#include "enser/causal_sequencer.h"

size_t encr_size(
    uint16_t refs_count,
    uint32_t payload_size
){
    return sizeof(encr_header_t)
        + ((size_t)refs_count * ENCR_HASH_SIZE)
        + payload_size;
}

int encr_build(
    const uint8_t **refs,
    uint16_t refs_count,
    const uint8_t *payload,
    uint32_t payload_size,
    uint8_t **out_buffer,
    size_t *out_size
){
    if (!out_buffer || !out_size)
        return -1;

    if (refs_count > 0 && !refs)
        return -2;

    size_t total =
        encr_size(
            refs_count,
            payload_size
        );

    uint8_t *buf =
        malloc(total);

    if (!buf)
        return -3;

    memset(buf, 0, total);

    encr_header_t *hdr =
        (encr_header_t *)buf;

    memcpy(
        hdr->magic,
        ENCR_MAGIC,
        4
    );

    hdr->version =
        ENCR_VERSION;

    hdr->refs_count =
        htons(refs_count);

    hdr->payload_size =
        htonl(payload_size);

    /* Get next causal sequence */
    uint64_t seq = caser_get_next();
    hdr->causal_seq = enser_htobe64(seq);

    /*
     * hash(payload)
     */
    if (payload_size > 0 && payload){
        hash_sha256(
            payload,
            payload_size,
            hdr->hash
        );
    }

    uint8_t *ptr =
        buf + sizeof(encr_header_t);

    /*
     * refs
     */
    for (
        uint16_t i = 0;
        i < refs_count;
        i++
    ){
        memcpy(
            ptr,
            refs[i],
            ENCR_HASH_SIZE
        );

        ptr += ENCR_HASH_SIZE;
    }

    /*
     * payload
     */
    if (payload_size > 0 && payload){
        memcpy(
            ptr,
            payload,
            payload_size
        );
    }

    *out_buffer = buf;
    *out_size = total;

    return ENCR_OK;
}

int encr_validate(
    const uint8_t *buffer,
    size_t size
){
    if (
        !buffer ||
        size < sizeof(encr_header_t)
    ){
        return ENCR_ERR_SIZE;
    }

    encr_header_t hdr;

    memcpy(
        &hdr,
        buffer,
        sizeof(hdr)
    );

    if (
        memcmp(
            hdr.magic,
            ENCR_MAGIC,
            4
        ) != 0
    ){
        return ENCR_ERR_MAGIC;
    }

    if (
        hdr.version != ENCR_VERSION
    ){
        return ENCR_ERR_VERSION;
    }

    uint16_t refs_count =
        ntohs(hdr.refs_count);

    uint32_t payload_size =
        ntohl(hdr.payload_size);

    size_t expected =
        encr_size(
            refs_count,
            payload_size
        );

    if (expected != size){
        return ENCR_ERR_CORRUPTED;
    }

    return ENCR_OK;
}

int encr_hash(
    const uint8_t *buffer,
    size_t size,
    uint8_t out[ENCR_HASH_SIZE]
){
    if (!buffer || !out)
        return -1;

    return hash_sha256(
        buffer,
        size,
        out
    );
}