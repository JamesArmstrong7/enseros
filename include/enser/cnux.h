#ifndef ENSER_CNUX_H
#define ENSER_CNUX_H

#include <stddef.h>
#include "enser/iyunux.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Represents a batched event from CNUX.
 *        Each event is assigned to a batch index for temporal scheduling.
 */
typedef struct {
    char *event;        // Canonical event string (from IYNUX)
    int batch_index;    // The batch in which this event should be executed
} cnux_batched_event_t;

typedef struct {
    cnux_batched_event_t *events; // Array of batched events
    size_t event_count;           // Number of events
} cnux_batched_graph_t;

/**
 * @brief Initialize the CNUX temporal metronome.
 * @param batch_rate_bps The batch rate in batches per second (e.g., 132 to 732).
 *                       This determines how much time each batch represents.
 * @return 0 on success, -1 on failure.
 */
int cnux_init(double batch_rate_bps);

/**
 * @brief Deinitialize the CNUX temporal metronome.
 */
void cnux_deinit(void);

/**
 * @brief Assign batch timing to an execution graph.
 * @param graph      The execution graph from IYNUX (list of canonical event strings).
 * @param batched_graph Pointer to a cnux_batched_graph_t that will be filled.
 *                       The caller must free the strings in the events array and the arrays themselves.
 * @return 0 on success, -1 on failure.
 *         On failure, *batched_graph will be set to an empty graph.
 */
int cnux_schedule_batches(const iynux_execution_graph_t *graph, cnux_batched_graph_t *batched_graph);

/**
 * @brief Get the current batch index based on the system's uptime.
 *        This function is used by the scheduler to know which batch we are in.
 * @return The current batch index (starting from 0 at initialization time).
 */
int cnux_get_current_batch_index(void);

#ifdef __cplusplus
}
#endif

#endif /* ENSer_CNUS_H */