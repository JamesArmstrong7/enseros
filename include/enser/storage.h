#ifndef ENSER_STORAGE_H
#define ENSER_STORAGE_H

#include <stddef.h>

int enser_storage_path(
    const char *hash,
    char *out,
    size_t out_size
);

int enser_storage_write(
    const char *hash,
    const void *data,
    size_t size
);

#endif
