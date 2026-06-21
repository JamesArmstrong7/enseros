#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enser/iyunux.h"
#include "enser/cromo.h"
#include "enser/pipo.h"
#include "enser/lba.h"
#include "enser/teddy.h"
#include "enser/encoder.h"
#include "enser/causal_sequencer.h"

extern int lba_pipo_init(const char *log_dir);
extern void lba_pipo_deinit(void);

int test_integrate(void) {
    // Initialize modules
    if (iynux_init() != 0) {
        fprintf(stderr, "iynux_init failed\n");
        return 1;
    }
    const char *log_dir = "./test_lba";
    if (lba_pipo_init(log_dir) != 0) {
        fprintf(stderr, "lba_pipo_init failed\n");
        iynux_deinit();
        return 1;
    }
    if (teddy_init(log_dir) != 0) {
        fprintf(stderr, "teddy_init failed\n");
        lba_pipo_deinit();
        iynux_deinit();
        return 1;
    }

    // Prepare intent
    const char *intent = "quiero suscribirme a un comic";

    // Compile intent via IYUNUX
    iynux_execution_graph_t graph;
    memset(&graph, 0, sizeof(graph));
    int ret = iynux_compile_intent(intent, NULL, &graph);
    if (ret != 0) {
        fprintf(stderr, "iynux_compile_intent failed\n");
        goto cleanup;
    }
    if (graph.event_count == 0) {
        fprintf(stderr, "iynux_compile_intent returned empty graph\n");
        goto cleanup;
    }
    const char *operation = graph.events[0];
    printf("First operation: %s\n", operation);

    // Process operation via CROMO
    lba_event_t **events = NULL;
    size_t out_event_count = 0;
    ret = cromo_process_intent(operation, NULL, "test_entity", 0.0, &events, &out_event_count);
    if (ret != 0) {
        fprintf(stderr, "cromo_process_intent failed\n");
        goto cleanup;
    }
    if (out_event_count != 1 || !events || !events[0]) {
        fprintf(stderr, "cromo_process_intent did not return exactly one event\n");
        goto cleanup;
    }
    lba_event_t *event = events[0];
    printf("Event ID: %s\n", event->event_id);
    printf("Event data: %s\n", event->data);

    // Serialize event through ENCR encoder
    const uint8_t *payload = (const uint8_t *)event->data;
    size_t payload_size = strlen(event->data);
    uint8_t *encr_buffer = NULL;
    size_t encr_size = 0;
    ret = encr_build(
        NULL, 0,
        payload, payload_size,
        &encr_buffer,
        &encr_size
    );
    if (ret != 0) {
        fprintf(stderr, "encr_build failed\n");
        goto cleanup;
    }
    printf("ENCR buffer size: %zu bytes\n", encr_size);

    // Optional: verify hash
    uint8_t hash[32];
    ret = encr_hash(encr_buffer, encr_size, hash);
    if (ret != 0) {
        fprintf(stderr, "encr_hash failed\n");
        goto cleanup;
    }

    // Append event via PIPO (LBA micro-agent)
    void *response = NULL;
    ret = pipo_handle_request(event, &response);
    if (ret != 0) {
        fprintf(stderr, "pipo_handle_request failed\n");
        goto cleanup;
    }
    // pipo_handle_request may have allocated a response (int*)
    if (response) {
        int *resp_int = response;
        if (*resp_int != 0) {
            fprintf(stderr, "PIPO response indicated failure\n");
        }
        free(response);
        response = NULL;
    }
    // After PIPO handling, the original event is no longer needed (the handler made a copy)
    // Free the event array and the event data (the event itself was allocated by cromo_process_intent)
    lba_event_free(event);
    free(events);
    events = NULL;
    out_event_count = 0;

    // Get causal sequence for debugging (optional)
    uint64_t casenum = caser_get_next();
    printf("Next causal sequence: %llu\n", (unsigned long long)casenum);

    // Replay via Teddy
    char *state = NULL;
    ret = teddy_replay_full(&state);
    if (ret != 0) {
        fprintf(stderr, "teddy_replay_full failed\n");
        goto cleanup;
    }
    printf("Replayed state: %s\n", state);

    // Validate that state contains the operation string
    if (strstr(state, operation) == NULL) {
        fprintf(stderr, "Replayed state does not contain the operation\n");
        goto cleanup;
    }
    // Also ensure it exactly matches operation (since we only have one event)
    if (strcmp(state, operation) != 0) {
        fprintf(stderr, "Replayed state does not exactly match operation\n");
        goto cleanup;
    }

    // Success
    ret = 0;

cleanup:
    // Cleanup resources
    if (encr_buffer) {
        free(encr_buffer);
    }
    if (state) {
        free(state);
    }
    if (events) {
        for (size_t i = 0; i < out_event_count; i++) {
            if (events[i]) {
                lba_event_free(events[i]);
            }
        }
        free(events);
    }
    if (graph.events) {
        for (size_t i = 0; i < graph.event_count; i++) {
            free(graph.events[i]);
        }
        free(graph.events);
    }
    // Deinitialize modules in reverse order
    lba_pipo_deinit();
    teddy_deinit();
    iynux_deinit();
    return ret;
}
