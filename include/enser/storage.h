/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSER_STORAGE_H
#define ENSER_STORAGE_H

#include <stddef.h>
#include <stdint.h>

int ensor_storage_write(const char *hash_hex, const uint8_t *event, size_t event_size);

#endif /* ENSER_STORAGE_H */
