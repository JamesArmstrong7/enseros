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