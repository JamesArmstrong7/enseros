#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <endian.h>

int encr_decoder(
    const uint8_t *buffer,
    size_t buffer_size,
    encr_object_t *out
){
    //Validar tamaño
    if (buffer_size < sizeof(encr_header_t))
        return ENCR_ERR_SIZE;

    // Leer header
    const encr_header_t *hdr =
    (const encr_header_t *)buffer;

    // Validar header
    if (memcmp(hdr->magic, "ENSR", 4) != 0)
        return ENCR_ERR_MAGIC;

    // Validar version
    if (hdr->version != ENCR_VERSION)
        return ENCR_ERR_VERSION;

    // Convertir endianess
    uint16_t refs_count =
    ntohs(hdr->refs_count);

    uint32_t payload_size =
    ntohl(hdr->payload_size);

    uint64_t timestamp =
    be64toh(hdr->timestamp);

    // Validar tamaños
    size_t expected =
    encoder_size(refs_count, payload_size);

    if (expected != buffer_size)
        return ENCR_ERR_CORRUPTED;

    // Reconstruir offsets
    const uint8_t *ptr =
    buffer + sizeof(encr_header_t);

    // Copiar payload
    out->payload = malloc(payload_size);

    memcpy(
        out->payload,
        ptr,
        payload_size
    );

    // Copiar metadata util
    out->header = *hdr;

    // Validacion de hash (futuro)
    TODO:
    recalculate hash(payload)
}
