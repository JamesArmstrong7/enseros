#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ENCR_OK = 0,

    ENCR_ERR_MAGIC = -1,
    ENCR_ERR_VERSION = -2,
    ENCR_ERR_SIZE = -3,
    ENCR_ERR_CORRUPTED = -4,
    ENCR_ERR_OVERFLOW = -5

} encr_result_t;

typedef struct __attribute__((packed)) {
    char magic[4];
    uint8_t version;

    uint16_t refs_count;
    uint32_t payload_size;

    uint64_t timestamp;

    uint8_t hash[32];
} encr_header_t;

typedef struct {
    encr_header_t header;

    uint8_t **refs;
    uint8_t *payload;

} encr_object_t;

int encr_decoder(
    const uint8_t *buffer,
    size_t buffer_size,
    encr_object_t *out
);

void encr_object_free(
    encr_object_t *obj
);
