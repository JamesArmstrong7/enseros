#ifndef ENSer_LBA_H
#define ENSer_LBA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Represents an event in the LBA (Log Base Append-only).
 */
typedef struct {
    char event_id[64];          // Format: {entity_id}_{timestamp_millis}
    char event_type[32];        // e.g., "TSP_REGISTERED", "ADMIN_REGISTERED", "USER_REGISTERED", "STATE_TRANSITIONED"
    double timestamp;           // Unix epoch time in seconds as double
    char *data;                 // Null-terminated JSON string representing the event payload
    int version;                // Version of the entity after this event
} lba_event_t;

/**
 * @brief Initializes the LBA module.
 * @param log_dir The directory where the log file will be stored. If NULL, defaults to "./lba".
 * @return 0 on success, -1 on failure.
 */
int lba_init(const char *log_dir);

/**
 * @brief Appends an event to the log.
 * @param event The event to append. The function will make a copy of the data string.
 *              The data string must be a valid JSON string.
 * @return 0 on success, -1 on failure.
 */
int lba_append(const lba_event_t *event);

/**
 * @brief Replays the entire log and returns an array of events.
 * @param out_count Pointer to an integer that will receive the number of events.
 * @return Pointer to an array of lba_event_t pointers, or NULL on failure.
 *         The caller must free each event with lba_event_free and then free the array.
 */
lba_event_t **lba_replay(size_t *out_count);

/**
 * @brief Replays the log and returns events for a specific entity.
 * @param entity_id The entity ID to filter by.
 * @param out_count Pointer to an integer that will receive the number of events.
 * @return Pointer to an array of lba_event_t pointers, or NULL on failure.
 *         The caller must free each event with lba_event_free and then free the array.
 */
lba_event_t **lba_get_events_by_entity(const char *entity_id, size_t *out_count);

/**
 * @brief Retrieves the latest event for a specific entity.
 * @param entity_id The entity ID to filter by.
 * @return Pointer to an lba_event_t, or NULL on failure or if no events found.
 *         The caller must free the event with lba_event_free.
 */
lba_event_t *lba_get_latest_event(const char *entity_id);

/**
 * @brief Frees an lba_event_t allocated by the LBA module.
 * @param event The event to free. If NULL, does nothing.
 */
void lba_event_free(lba_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* ENSer_LBA_H */