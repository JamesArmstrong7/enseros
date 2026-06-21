#ifndef ENSer_TEDDY_H
#define ENSer_TEDDY_H

#include <stddef.h>
#include "enser/lba.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Initialize Teddy with the LBA directory.
 * @param lba_dir  The directory where the LBA log file is stored (e.g., "./lba").
 *                 The LBA log file is expected to be named "lba.jsonl" inside this directory.
 * @return 0 on success, -1 on failure.
 */
int teddy_init(const char *lba_dir);

/**
 * @brief Deinitialize Teddy, releasing any resources.
 */
void teddy_deinit(void);

/**
 * @brief Replay the entire LBA and reconstruct the state.
 * @param state  Pointer to a pointer that will receive the state string.
 *               The state string is a concatenation of all payloads from the LBA, separated by newlines.
 *               The caller must free the state string.
 * @return 0 on success, -1 on failure.
 */
int teddy_replay_full(char **state);

/**
 * @brief Replay the LBA and reconstruct the state for a specific entity.
 * @param entity_id  The entity ID to filter by.
 * @param state      Pointer to a pointer that will receive the state string for the entity.
 *                   The state string is a concatenation of payloads from LBA events that pertain to the entity,
 *                   separated by newlines.
 *                   The caller must free the state string.
 * @return 0 on success, -1 on failure.
 */
int teddy_replay_entity(const char *entity_id, char **state);

/**
 * @brief Replay the LBA and reconstruct the state for a specific resource.
 *        Note: This function is not fully implemented in this version as the resource concept
 *        is not explicitly defined in the LBA payload. It behaves similarly to teddy_replay_entity.
 * @param resource_id  The resource ID to filter by.
 * @param state        Pointer to a pointer that will receive the state string for the resource.
 *                     The caller must free the state string.
 * @return 0 on success, -1 on failure.
 */
int teddy_replay_resource(const char *resource_id, char **state);

#ifdef __cplusplus
}
#endif

#endif /* ENSer_TEDDY_H */