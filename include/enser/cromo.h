/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSer_CROMO_H
#define ENSer_CROMO_H

#include <stddef.h>
#include "enser/lba.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Process an intent and produce a list of events to append to the LBA.
 *        This function validates the intent, canonicalizes it into operations,
 *        synthesizes a single transaction event, and prepares it for appending.
 *
 * @param intent      The user's intent string (e.g., "grant access to resource X").
 * @param current_state The current state string as reconstructed by Teddy (e.g., JSON).
 * @param entity_id   The entity ID for which the intent is directed (used for event ID generation).
 * @param timestamp   The timestamp to use for the event (Unix epoch seconds as double).
 * @param events      Pointer to a pointer that will receive an array of lba_event_t pointers.
 *                    The array will contain exactly one event (the transaction event).
 *                    The caller must free the array and the lba_event_t inside it using lba_event_free.
 * @param out_event_count Pointer to an integer that will receive the number of events (always 1 on success).
 * @return 0 on success, -1 on failure.
 *         On failure, *events will be set to NULL and *out_event_count to 0.
 */
int cromo_process_intent(const char *intent, const char *current_state,
                         const char *entity_id, double timestamp,
                         lba_event_t ***events, size_t *out_event_count);

#ifdef __cplusplus
}
#endif

#endif /* ENSer_CROMO_H */