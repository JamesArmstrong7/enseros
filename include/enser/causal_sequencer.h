/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSER_CAUSAL_SEQUENCER_H
#define ENSER_CAUSAL_SEQUENCER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t caser_get_next(void);
void caser_reset(void);

#ifdef __cplusplus
}
#endif

#endif