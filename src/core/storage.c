#include "enser/storage.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int ensor_storage_write(const char *hash_hex, const uint8_t *event, size_t event_size) {
    if (!hash_hex || !event || event_size == 0) {
        fprintf(stderr, "storage write: null ptr or zero size\n");
        return -1;
    }

    // Expect hash_hex to be 64 hex characters (lowercase)
    if (strlen(hash_hex) != 64) {
        fprintf(stderr, "storage write: hash length not 64, got %zu\n", strlen(hash_hex));
        return -1;
    }

    // Validate that hash_hex contains only hex digits
    for (size_t i = 0; i < 64; i++) {
        char c = hash_hex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            fprintf(stderr, "storage write: invalid hex char at %zu: %c\n", i, c);
            return -1;
        }
    }

    // Ensure base storage directory exists
    if (mkdir("storage", 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "storage write: mkdir failed for storage: %s\n", strerror(errno));
        return -1;
    }

    // Construct path: storage/<first two>/<rest>
    char path[256];
    snprintf(path, sizeof(path), "storage/%c%c/%s", hash_hex[0], hash_hex[1], hash_hex + 2);

    // Create directory if it doesn't exist
    char dir[256];
    snprintf(dir, sizeof(dir), "storage/%c%c", hash_hex[0], hash_hex[1]);
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "storage write: mkdir failed for %s: %s\n", dir, strerror(errno));
        return -1;
    }

    // Open file for writing (binary)
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "storage write: fopen failed for %s: %s\n", path, strerror(errno));
        return -1;
    }

    // Write event data
    size_t written = fwrite(event, 1, event_size, fp);
    fclose(fp);

    if (written != event_size) {
        fprintf(stderr, "storage write: incomplete write, written %zu of %zu\n", written, event_size);
        // Optionally remove incomplete file
        unlink(path);
        return -1;
    }

    return 0;
}
