/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include "enser/cnux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Static variables to track CNUX state
static double batch_rate_bps = 0.0;
static time_t start_time = 0;

int cnux_init(double batch_rate_bps_param) {
    // Validate batch rate (between 132 and 732 batches per second as per manifest)
    if (batch_rate_bps_param < 132.0 || batch_rate_bps_param > 732.0) {
        return -1;
    }

    batch_rate_bps = batch_rate_bps_param;
    start_time = time(NULL);

    return 0;
}

void cnux_deinit(void) {
    // No state to free
    batch_rate_bps = 0.0;
    start_time = 0;
}

int cnux_schedule_batches(const iynux_execution_graph_t *graph, cnux_batched_graph_t *batched_graph) {
    if (!graph || !batched_graph) {
        return -1;
    }

    // Initialize output graph to empty
    batched_graph->events = NULL;
    batched_graph->event_count = 0;

    if (!graph->events || graph->event_count == 0) {
        return 0; // Nothing to schedule
    }

    // Allocate memory for batched events
    batched_graph->events = malloc(graph->event_count * sizeof(cnux_batched_event_t));
    if (!batched_graph->events) {
        return -1;
    }

    // Assign sequential batch indices starting from 0
    for (size_t i = 0; i < graph->event_count; i++) {
        batched_graph->events[i].event = strdup(graph->events[i]);
        if (!batched_graph->events[i].event) {
            // Clean up on failure
            for (size_t j = 0; j < i; j++) {
                free(batched_graph->events[j].event);
            }
            free(batched_graph->events);
            batched_graph->events = NULL;
            batched_graph->event_count = 0;
            return -1;
        }
        batched_graph->events[i].batch_index = (int)i;
    }

    batched_graph->event_count = graph->event_count;
    return 0;
}

int cnux_get_current_batch_index(void) {
    if (batch_rate_bps <= 0.0 || start_time == 0) {
        return 0;
    }

    time_t current_time = time(NULL);
    double elapsed_seconds = difftime(current_time, start_time);
    int batch_index = (int)(elapsed_seconds * batch_rate_bps);

    return batch_index;
}