[ARCHITECTURE SPEC]

Module:
teddy (Teddy - Historical Interpreter and State Reconstructor)

Dependencies:
- lba.h (for reading the LBA)
- stdio.h, stdlib.h, string.h, unistd.h, sys/stat.h, fcntl.h, arpa/inet.h, stdint.h, stddef.h, stdbool.h, errno.h

Memory Budget:
- 64 MB (for Teddy's state during reconstruction; note that Teddy can reconstruct incrementally to avoid high memory usage)

ABI Stability:
- Stable: the Teddy interface must remain stable for consumers (such as CROMO and PIPO Daemon).

Determinism Class:
- Deterministic: given the same LBA, Teddy will always reconstruct the same state.

[PUBLIC TYPES]

// We reuse the lba_event_t from lba.h for events.

// Teddy's main function is to replay the LBA and reconstruct the state.
// We define a state as an opaque string (e.g., a JSON string) for simplicity.
// In practice, the state could be a complex structure, but we represent it as a string to keep the interface generic.

// We also define functions for different reconstruction modes: full replay, delta replay, entity replay, etc.

// We will define a Teddy context that holds the state during reconstruction.
// However, to keep the interface simple and stateless, we will provide functions that take the LBA directory and return the state.

// Alternatively, we can have Teddy maintain a connection to the LBA and incrementally update.
// But given the memory constraints and the need for determinism, we will provide a replay function that reconstructs the state from scratch.

// We define:

typedef struct {
    // Function to initialize Teddy with the LBA directory.
    // Input:
    //   lba_dir: the directory where the LBA log file is stored (e.g., "./lba")
    // Output:
    //   None
    // Returns:
    //   0 on success, -1 on failure
    int (*init)(const char *lba_dir);

    // Function to deinitialize Teddy, freeing any resources.
    // Input:
    //   None
    // Output:
    //   None
    // Returns:
    //   void
    void (*deinit)(void);

    // Function to reconstruct the state by replaying the entire LBA.
    // Input:
    //   None (uses the LBA directory from init)
    // Output:
    //   state: a pointer to a string that represents the reconstructed state (e.g., JSON)
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the state string.
    int (*replay_full)(char **state);

    // Function to reconstruct the state for a specific entity.
    // Input:
    //   entity_id: the ID of the entity to reconstruct
    // Output:
    //   state: a pointer to a string that represents the state of the entity
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the state string.
    int (*replay_entity)(const char *entity_id, char **state);

    // Function to reconstruct the state for a specific resource.
    // Input:
    //   resource_id: the ID of the resource to reconstruct
    // Output:
    //   state: a pointer to a string that represents the state of the resource
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the state string.
    int (*replay_resource)(const char *resource_id, char **state);

    // Function to rebuild the state incrementally from a given timestamp.
    // This is useful for reducing replay cost when only recent changes are needed.
    // Input:
    //   since_timestamp: the timestamp (Unix epoch seconds as double) from which to start replaying
    // Output:
    //   state_delta: a pointer to a string that represents the changes since the given timestamp
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the state_delta string.
    int (*replay_since)(double since_timestamp, char **state_delta);

    // Function to detect divergence between an observed state and the rebuilt state.
    // Input:
    //   observed_state: a string representing the observed state (e.g., from memory)
    //   rebuilt_state: a string representing the state rebuilt by Teddy from the LBA
    // Output:
    //   divergence: a pointer to a string that describes the divergence (or NULL if none)
    // Returns:
    //   0 on success, -1 on failure
    //   The caller must free the divergence string.
    int (*detect_divergence)(const char *observed_state, const char *rebuilt_state, char **divergence);
} teddy_interface_t;

// Note: The above interface assumes that Teddy is initialized with an LBA directory.
// We must ensure that the LBA module is compatible (i.e., the LBA log file is in the expected format).

// We also note that Teddy can be used to answer temporal queries:
//   What was the state of entity X at time T?
// This can be done by replaying the LBA up to time T.
// We can provide a function for that if needed.

[INVARIANTS]

- Teddy must never modify the LBA.
- Teddy's reconstruction must be deterministic: same LBA input yields same state output.
- Teddy must handle LBA files that are append-only and never modified.
- Teddy must be able to handle large LBA files by replaying incrementally (to avoid loading the entire file into memory).
- Teddy must not store any persistent state between calls (except for the LBA directory path, which is set at init).

[API CONTRACT]

Function:
teddy_init
Ownership:
  The caller must call teddy_init with the LBA directory before using any other function.
Failure modes:
  Returns -1 if the LBA directory is invalid or if the LBA log file cannot be opened for reading.
  Returns 0 on success.
Determinism guarantees:
  Initialization is deterministic.

Function:
teddy_deinit
Ownership:
  Frees any resources allocated by Teddy.
Failure modes:
  None.
Determinism guarantees:
  Deterministic.

Function:
teddy_replay_full
Ownership:
  The caller owns the returned state string and must free it.
Failure modes:
  Returns -1 if the LBA cannot be read or if memory allocation fails.
  Returns 0 on success (and returns a state string).
Determinism guarantees:
  Given the same LBA, the state string will always be the same.

Function:
teddy_replay_entity
Ownership:
  The caller owns the returned state string and must free it.
Failure modes:
  Returns -1 if the entity_id is invalid or if the LBA cannot be read.
  Returns 0 on success.
Determinism guarantees:
  Given the same LBA and entity_id, the state string will always be the same.

[HANDOFF]

Target Agent:
C-Implementer
