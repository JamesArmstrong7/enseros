[ARCHITECTURE SPEC]

Module:
cromo (CROMO - Active Mutation Engine)

Dependencies:
- lba.h (for appending events to the LBA)
- enser/encr.h (for event encryption and hashing, if needed)
- stdio.h, stdlib.h, string.h, unistd.h, sys/stat.h, fcntl.h, arpa/inet.h, stdint.h, stddef.h, stdbool.h, errno.h

Memory Budget:
- 64 MB (for the CROMO engine, which must be lightweight and not hold large state)

ABI Stability:
- Stable: the CROMO interface must remain stable for consumers (such as PIPO and governance layers).

Determinism Class:
- Deterministic: given the same intent and the same LBA state, CROMO will produce the same events and append them in the same order.

[PUBLIC TYPES]

// We define the intent structure that CROMO receives.
// This is a simplified representation; in practice, the intent would be domain-specific.
// For the purpose of this architecture, we assume the intent is a string that describes the action.
// However, CROMO must be able to validate, canonicalize, and synthesize events.

// We also define the event structure that CROMO produces (which is the same as the LBA event).
// But note: the LBA event we defined earlier is in lba.h. We will reuse that.

// However, note that the LBA event in lba.h is designed for the LBA module (which we already implemented).
// We must ensure compatibility.

// We will use the lba_event_t from lba.h.

// CROMO's main function is to take an intent and produce a list of lba_event_t events to append to the LBA atomically.

// We also need to consider validation: CROMO must check permissions, constraints, dependencies, etc.
// This implies that CROMO needs access to the current state (which Teddy provides via LBA replay).
// However, to avoid coupling, we can design CROMO to receive the current state as an input (from the caller, which would have gotten it from Teddy).

// Alternatively, we can have CROMO call Teddy to reconstruct the state. But this would create a dependency cycle.

// Given the architecture, we assume that the caller (e.g., the governance layer or PIPO after validation) has already reconstructed the state using Teddy and then passes the state to CROMO along with the intent.

// Therefore, we define the CROMO interface as:

typedef struct {
    // Function to validate and canonicalize an intent into a list of operations.
    // Input:
    //   intent: a string describing the user's intent (e.g., "grant access to resource X for user Y")
    //   current_state: a string representing the current state (as reconstructed by Teddy, e.g., a JSON string)
    // Output:
    //   operations: a pointer to an array of strings, each string being a canonical operation (e.g., "GRANT_PERMISSION")
    //   out_operation_count: the number of operations
    // Returns:
    //   0 on success, -1 on failure (e.g., invalid intent, missing permissions)
    //   The caller must free the operations array and the strings inside it.
    int (*validate_and_canonicalize)(const char *intent, const char *current_state, char ***operations, size_t *out_operation_count);

    // Function to synthesize events from the canonical operations.
    // Input:
    //   operations: array of canonical operation strings
    //   operation_count: number of operations
    //   entity_id: the ID of the entity on which the mutation is performed (for generating event IDs)
    //   timestamp: the timestamp to use for the events (Unix epoch seconds as double)
    // Output:
    //   events: a pointer to an array of lba_event_t pointers, each representing an event to append to the LBA
    //   out_event_count: the number of events
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the events array and the lba_event_t structures inside it (using lba_event_free).
    int (*synthesize_events)(char **operations, size_t operation_count, const char *entity_id, double timestamp, lba_event_t ***events, size_t *out_event_count);

    // Function to append the events atomically to the LBA.
    // Input:
    //   events: array of lba_event_t pointers
    //   event_count: number of events
    // Output:
    //   None
    // Returns:
    //   0 on success (all events appended), -1 on failure (if any event fails to append, none are appended)
    int (*append_events_atomically)(lba_event_t **events, size_t event_count);
} cromo_interface_t;

// However, note that the above interface splits the process into three steps.
// We might also provide a single-step function that does validation, canonicalization, synthesis, and atomic append.
// But for flexibility and to allow the caller to handle validation errors separately, we keep it split.

// Alternatively, we can think of CROMO as a black box that takes intent and current state and returns events to append.
// We'll provide a simpler interface for the most common case:

typedef struct {
    // Function to process an intent and return events to append to the LBA atomically.
    // Input:
    //   intent: the user's intent string
    //   current_state: the current state string (as reconstructed by Teddy)
    //   entity_id: the entity ID for event ID generation
    //   timestamp: the timestamp for the events
    // Output:
    //   events: a pointer to an array of lba_event_t pointers (the events to append)
    //   out_event_count: the number of events
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the events array and the lba_event_t structures.
    int (*process_intent)(const char *intent, const char *current_state, const char *entity_id, double timestamp, lba_event_t ***events, size_t *out_event_count);
} cromo_simple_interface_t;

// We will implement the simple interface and let the caller use it.
// The complex interface can be built on top of the simple one if needed.

[INVARIANTS]

- CROMO must never modify the intent or current_state inputs.
- CROMO must ensure that the events it produces are valid LBA events (as per the lba_event_t structure and the LBA invariants).
- CROMO must ensure that the events are appended atomically (either all succeed or none are appended).
- CROMO must not store any state between calls (it is stateless).
- CROMO must enforce that all mutations go through the LBA (no bypass).

[API CONTRACT]

Function:
cromo_init_simple
Ownership:
  The caller must initialize the cromo_simple_interface_t structure.
  We provide a constructor that sets the function pointers.
Failure modes:
  Returns NULL if memory allocation fails for the interface structure.
  Returns a pointer to the interface on success.
Determinism guarantees:
  Initialization is deterministic.

Function:
cromo_process_intent
Ownership:
  The caller provides the intent, current_state, entity_id, and timestamp.
  CROMO returns an array of lba_event_t pointers (events) that must be appended to the LBA.
  The caller must append the events using the LBA module's append function (or use cromo's append_events_atomically if we provide it).
  The caller must free the events array and the lba_event_t structures.
Failure modes:
  Returns -1 if the intent is invalid, if current_state is NULL, or if any internal step fails.
  Returns 0 on success.
Determinism guarantees:
  Given the same intent, current_state, entity_id, and timestamp, CROMO will always produce the same list of events.

[HANDOFF]

Target Agent:
C-Implementer
