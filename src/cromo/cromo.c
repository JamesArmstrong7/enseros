#include "enser/cromo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "enser/lba.h"

int cromo_process_intent(const char *intent, const char *current_state,
                         const char *entity_id, double timestamp,
                         lba_event_t ***events, size_t *out_event_count) {
    if (!intent || !entity_id || !events || !out_event_count) {
        return -1;
    }
    (void)current_state; // suppress unused parameter warning for now

    // Step 1: Validate and canonicalize the intent.
    // For simplicity, we treat the intent as a single operation and canonicalize it to itself.
    // In a real system, we would check permissions, constraints, etc., using the current_state.
    // We will do a basic validation: intent must not be empty.
    if (intent[0] == '\0') {
        return -1;
    }

    // We will create an array of operations (for now, just one).
    const char *operation = intent;

    // Step 2: Synthesize a single transaction event.
    lba_event_t *event = malloc(sizeof(lba_event_t));
    if (!event) {
        return -1;
    }

    // Generate event_id: {entity_id}_{timestamp_millis}
    long long timestamp_millis = (long long)(timestamp * 1000.0);
    int len = snprintf(NULL, 0, "%s_%lld", entity_id, timestamp_millis);
    if (len < 0 || len >= (int)sizeof(event->event_id)) {
        free(event);
        return -1;
    }
    snprintf(event->event_id, sizeof(event->event_id), "%s_%lld", entity_id, timestamp_millis);

    // Set event_type
    strncpy(event->event_type, "TRANSACTION", sizeof(event->event_type)-1);
    event->event_type[sizeof(event->event_type)-1] = 0;

    // Set timestamp
    event->timestamp = timestamp;

    // Create the payload: a JSON string.
    // We need to escape quotes and backslashes in the operation.
    size_t payload_len = 2; // for the surrounding quotes
    for (size_t i = 0; operation[i]; i++) {
        if (operation[i] == '"' || operation[i] == '\\') {
            payload_len++; // escape character
        }
        payload_len++;
    }
    char *payload = malloc(payload_len + 1); // +1 for null terminator
    if (!payload) {
        free(event);
        return -1;
    }
    payload[0] = '"';
    size_t pos = 1;
    for (size_t i = 0; operation[i]; i++) {
        if (operation[i] == '"') {
            payload[pos++] = '\\';
            payload[pos++] = '"';
        } else if (operation[i] == '\\') {
            payload[pos++] = '\\';
            payload[pos++] = '\\';
        } else {
            payload[pos++] = operation[i];
        }
    }
    payload[pos] = '"';
    payload[pos+1] = '\0';

    event->data = payload;
    event->version = 0; // Let the LBA replay handle versioning

    // Step 3: Prepare the event for returning.
    // We will return an array of one event.
    lba_event_t **event_array = malloc(sizeof(lba_event_t *));
    if (!event_array) {
        free(event->data);
        free(event);
        return -1;
    }
    event_array[0] = event;

    *events = event_array;
    *out_event_count = 1;

    return 0;
}