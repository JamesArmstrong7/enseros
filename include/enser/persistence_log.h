/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSER_PERSISTENCE_LOG_H
#define ENSER_PERSISTENCE_LOG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int plog_init(const char *log_dir);
void plog_deinit(void);
int plog_append(const uint8_t *event, size_t event_size);
int plog_read_next(uint8_t **out_event, size_t *out_size);
void plog_reset_read(void);

#ifdef __cplusplus
}
#endif

#endif