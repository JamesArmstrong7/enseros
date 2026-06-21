[ARCHITECTURE SPEC]

Module:
pipo (PIPO - Micro-Agent Layer)

Dependencies:
- None (only standard C libraries: stdio.h, stdlib.h, string.h, unistd.h, sys/socket.h, netinet/in.h, arpa/inet.h, stdint.h, stddef.h, stdbool.h, errno.h, pthread.h? but we must avoid threads unless specified; however, PIPO Daemon might use threads, but the PIPO micro-agent itself is lightweight and may not need threads)

Memory Budget:
- 64 MB (for the PIPO micro-agent, which should be lightweight)

ABI Stability:
- Stable: the PIPO interface must remain stable for modules to depend on.

Determinism Class:
- Deterministic: given the same input, the PIPO micro-agent will make the same decision (escalate or handle locally).

[PUBLIC TYPES]

// We define a simple message structure for communication between PIPO and PIPO Daemon.
// However, the PIPO micro-agent itself is embedded in the module and does not directly use these.
// The PIPO Daemon uses sockets for communication.

// For the purpose of this architecture, we define the PIPO micro-agent's interface with the module it protects.

// The PIPO micro-agent has two main functions:
// 1. handle_local_request: for requests that can be handled within the module (Fast Path)
// 2. escalate_to_daemon: for requests that require domain coordination (Daemon Path) or higher.

// We do not define the internal structure of the PIPO micro-agent here, only its contract with the module.

// The module must provide a way to register its PIPO handlers.

typedef struct {
    // Function to handle a request locally (Fast Path)
    // Returns 0 if handled, -1 if escalation is needed.
    int (*handle_local)(const void *request, void **response);

    // Function to escalate a request to the PIPO Daemon
    // This function would typically send the request over a socket to the daemon.
    // Returns 0 on success (escalation initiated), -1 on failure.
    int (*escalate)(const void *request);

    // Optional: function to handle a response from the daemon (for synchronous escalation)
    // This is used if the module waits for a response from the daemon.
    int (*handle_daemon_response)(const void *daemon_response, void **response);
} pipo_interface_t;

// The module must initialize its PIPO by providing the above interface.
// The PIPO micro-agent will then use these functions to process incoming requests.

[INVARIANTS]

- The PIPO micro-agent is the only entry point to the module.
- No module function can be called directly without going through the PIPO.
- The PIPO micro-agent must never allow a request to bypass its handling.
- The PIPO micro-agent must enforce that only authenticated and authorized requests are processed (based on module-specific policies).
- The PIPO micro-agent must prioritize Fast Path handling to minimize latency.

[API CONTRACT]

Function:
pipo_init
Ownership:
  The module must call pipo_init to initialize its PIPO.
  The module provides the pipo_interface_t structure.
Failure modes:
  Returns -1 if the interface is NULL or if required functions are missing.
  Returns 0 on success.
Determinism guarantees:
  Initialization is deterministic.

Function:
pipo_handle_request
Ownership:
  This function is called by the module's entry point (e.g., when a socket receives a connection, or when a function is called internally).
  The PIPO micro-agent takes over and decides whether to handle locally or escalate.
  The module does not need to free the request or response; ownership is managed by the PIPO micro-agent according to the interface.
Failure modes:
  Returns -1 if the PIPO is not initialized or if there is an internal error.
  Returns 0 on success (the request was handled, either locally or via escalation, and a response was generated).
Determinism guarantees:
  Given the same request and the same module state, the PIPO will always make the same decision (handle locally or escalate) and produce the same response.

[HANDOFF]

Target Agent:
C-Implementer
