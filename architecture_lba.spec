[ARCHITECTURE SPEC]

Module:
lba (Log Base Append-only)

Dependencies:
- None (only standard C libraries: stdio.h, stdlib.h, string.h, unistd.h, sys/stat.h, fcntl.h, arpa/inet.h, stdint.h, stddef.h, stdbool.h)

Memory Budget:
- 64 MB (for the log file and in-memory caching during replay, but the log file can grow beyond this as it is stored on disk; the memory budget is for runtime structures)

ABI Stability:
- Stable: the event structure and the replay function must maintain backward compatibility.

Determinism Class:
- Deterministic: given the same sequence of input events, the replay function will always produce the same state.

[PUBLIC TYPES]

typedef struct {
    char event_id[64];          // Format: {entity_id}_{timestamp_millis}
    char event_type[32];        // e.g., "TSP_REGISTERED", "ADMIN_REGISTERED", "USER_REGISTERED", "STATE_TRANSITIONED"
    double timestamp;           // Unix epoch time in seconds as double
    // data is a JSON object, but we will store it as a string for simplicity and flexibility
    char *data;                 // Null-terminated JSON string
    int version;                // Version of the entity after this event
} lba_event_t;

// Note: The actual storage format is a line of JSON per event in a file.
// The in-memory representation is the above struct for ease of use.

// We also define a function to free the event.
void lba_event_free(lba_event_t *event);

[INVARIANTS]

- The log file is append-only: no event is ever modified or removed.
- Each event has a unique event_id.
- The timestamp of events is non-decreasing (monotonic) for a given entity.
- The version in an event is strictly greater than the previous version for the same entity.
- The data field is a valid JSON string.

[API CONTRACT]

Function:
lba_init
Ownership:
  The caller must call lba_init before any other function.
Failure modes:
  Returns -1 if the log directory cannot be created or the log file cannot be opened for appending.
  Returns 0 on success.
Determinism guarantees:
  Initializes the log subsystem deterministically.

Function:
lba_append
Ownership:
  The caller owns the lba_event_t passed in and must free it after the call if needed (the function makes a copy of the data string).
  The function takes ownership of the data string? We will copy the data string and store it in the log.
  To avoid ownership confusion, we will design the function to take a const lba_event_t * and copy the data.
Failure modes:
  Returns -1 if the event is NULL or if writing to the log file fails.
  Returns 0 on success.
Determinism guarantees:
  Appending an event is deterministic and will always result in the same log file state.

Function:
lba_replay
Ownership:
  The caller must free the returned array and the events inside it.
  The function returns an array of lba_event_t pointers and the number of events.
  The caller must call lba_event_free on each event and then free the array.
Failure modes:
  Returns NULL if the log file cannot be opened for reading or if memory allocation fails.
  Returns an array of events on success (even if empty array).
Determinism guarantees:
  Replaying the log will always produce the same sequence of events in the same order.

Function:
lba_get_events_by_entity
Ownership:
  Similar to lba_replay, but filtered by entity_id.
  The caller must free the returned array and the events inside it.
Failure modes:
  Returns NULL if the log file cannot be opened for reading or if memory allocation fails.
  Returns an array of events on success.
Determinism guarantees:
  Returns events in the order they appear in the log (which is chronological).

Function:
lba_get_latest_event
Ownership:
  The caller must free the returned event.
Failure modes:
  Returns NULL if no events are found for the entity or if the log file cannot be opened.
  Returns a pointer to an lba_event_t on success.
Determinism guarantees:
  Returns the latest event (by timestamp) for the entity.

Function:
lba_event_free
Ownership:
  Frees the memory allocated for an lba_event_t, including the data string.
Failure modes:
  None (if the event is NULL, it does nothing).
Determinism guarantees:
  Always frees the memory.

[HANDOFF]

Target Agent:
C-Implementer