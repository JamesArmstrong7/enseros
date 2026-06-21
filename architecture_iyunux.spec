[ARCHITECTURE SPEC]

Module:
iyunux (Motor de Gobernanza)

Dependencies:
- lba.h (for appending events)
- enser/encr.h (maybe not needed, but we might need to create events)
- stdio.h, stdlib.h, string.h, etc.

Memory Budget:
- 64 MB (for storing governance rules and transient state)

ABI Stability:
- Stable: the function signatures for validation and transition execution must remain stable.

Determinism Class:
- Deterministic: given the same state and proposed transition, the validation result will always be the same.

[PUBLIC TYPES]

typedef struct {
    const char *name;
    bool (*condition)(const void *before_state, const void *after_state);
    const char *description;
} iyunux_governance_rule_t;

typedef struct {
    iyunux_governance_rule_t *rules;
    size_t rule_count;
    size_t capacity;
} iyunux_governance_t;

/**
 * @brief Initializes the IYNUX governance module.
 * @param gov Pointer to an iyunux_governance_t to initialize.
 * @return 0 on success, -1 on failure.
 */
int iyunux_init(iyunux_governance_t *gov);

/**
 * @brief Deinitializes the IYNUX governance module, freeing any allocated memory.
 * @param gov Pointer to an iyunux_governance_t to deinitialize.
 */
void iyunux_deinit(iyunux_governance_t *gov);

/**
 * @brief Adds a governance rule to the module.
 * @param gov Pointer to an iyunux_governance_t.
 * @param rule The rule to add (the module will make a copy of the name and description).
 * @return 0 on success, -1 on failure.
 */
int iyunux_add_rule(iyunux_governance_t *gov, const iyunux_governance_rule_t *rule);

/**
 * @brief Validates a transition between two states.
 * @param gov Pointer to an iyunux_governance_t.
 * @param before_state Pointer to the before state (opaque, interpreted by the rule condition).
 * @param after_state Pointer to the after state (opaque, interpreted by the rule condition).
 * @param out_violated_rules Pointer to an array of strings that will be filled with the names of violated rules.
 * @param out_violated_count Pointer to an integer that will receive the number of violated rules.
 * @return 0 if the transition is valid (no violated rules), -1 if invalid (one or more violated rules).
 *         The caller must free the strings in out_violated_rules and the array itself.
 */
int iyunux_valid_transition(const iyunux_governance_t *gov,
                            const void *before_state,
                            const void *after_state,
                            char ***out_violated_rules,
                            size_t *out_violated_count);

/**
 * @brief Executes a transition if valid.
 * @param gov Pointer to an iyunux_governance_t.
 * @param before_state Pointer to the before state.
 * @param after_state Pointer to the after state.
 * @param context Additional context for the transition (opaque).
 * @param out_event Pointer to an lba_event_t that will be filled if the transition is valid.
 * @return 0 if the transition was executed and an event was created, -1 if invalid.
 *         The caller must free the returned event with lba_event_free.
 */
int iyunux_execute_transition(const iyunux_governance_t *gov,
                              const void *before_state,
                              const void *after_state,
                              const void *context,
                              lba_event_t **out_event);

[INVARIANTS]

- The governance rules are checked in the order they were added.
- A transition is valid only if all rules return true.
- The module does not mutate the before_state or after_state.

[API CONTRACT]

Function:
iyunux_init
Ownership:
  The caller must initialize the iyunux_governance_t before use.
Failure modes:
  Returns -1 if memory allocation fails.
  Returns 0 on success.
Determinism guarantees:
  Initialization is deterministic.

Function:
iyunux_deinit
Ownership:
  Frees all memory associated with the governance rules.
Failure modes:
  None.
Determinism guarantees:
  Deterministic.

Function:
iyunux_add_rule
Ownership:
  The module makes a copy of the rule's name and description.
  The caller retains ownership of the passed rule.
Failure modes:
  Returns -1 if memory allocation fails for the rule copies or for expanding the rules array.
  Returns 0 on success.
Determinism guarantees:
  Deterministic.

Function:
iyunux_valid_transition
Ownership:
  The caller owns the before_state and after_state (opaque pointers).
  The module will allocate memory for the out_violated_rules array and the strings inside it.
  The caller must free the strings and the array.
Failure modes:
  Returns -1 if the transition is invalid (one or more rules violated) or if memory allocation fails.
  Returns 0 if the transition is valid (no violated rules).
Determinism guarantees:
  Deterministic: same inputs yield same output.

Function:
iyunux_execute_transition
Ownership:
  The caller owns the before_state, after_state, and context (opaque).
  The module will create and return a new lba_event_t if the transition is valid.
  The caller must free the returned event with lba_event_free.
Failure modes:
  Returns -1 if the transition is invalid or if memory allocation fails for the event.
  Returns 0 on success (and the event is returned via out_event).
Determinism guarantees:
  Deterministic: same inputs yield same output event.

[HANDOFF]

Target Agent:
C-Implementer