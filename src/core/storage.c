#include "enser/storage.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int enser_storage_path(
    const char *hash,
    char *out,
    size_t out_size
) {
    if (!hash || strlen(hash) < 4) {
        return -1;
    }

    return snprintf(
        out,
        out_size,
        "storage/%.2s/%.2s/%s",
        hash,
        hash + 2,
        hash
    );
}

int enser_storage_write(
    const char *hash,
    const void *data,
    size_t size
) {
    char path[256];

    if (enser_storage_path(hash, path, sizeof(path)) < 0) {
        return -1;
    }

    char dir1[64];
    char dir2[64];

    snprintf(dir1, sizeof(dir1), "storage/%.2s", hash);

    snprintf(
        dir2,
        sizeof(dir2),
        "storage/%.2s/%.2s",
        hash,
        hash + 2
    );

    mkdir("storage", 0755);
    mkdir(dir1, 0755);
    mkdir(dir2, 0755);

    FILE *fp = fopen(path, "wb");

    if (!fp) {
        return -1;
    }

    fwrite(data, 1, size, fp);

    fclose(fp);

    return 0;
}
