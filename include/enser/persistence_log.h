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