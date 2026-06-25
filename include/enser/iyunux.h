/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSer_IYNUX_H
#define ENSer_IYNUX_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Represents an execution graph from the IYNUX intent compiler.
 *        Each event is a canonical string representing an operation.
 */
typedef struct {
    char **events;      // Array of strings, each string is a canonical event (e.g., "CREATE_SUBSCRIPTION")
    size_t event_count; // Number of events
} iynux_execution_graph_t;

/**
 * @brief Initialize the IYNUX intent compiler.
 * @return 0 on success, -1 on failure.
 */
int iynux_init(void);

/**
 * @brief Deinitialize the IYNUX intent compiler.
 *        Frees any allocated resources.
 */
void iynux_deinit(void);

/**
 * @brief Compile user intent into an execution graph.
 * @param intent       The user's intent string (e.g., "quiero suscribirme a un comic").
 * @param system_state The current system state string (as reconstructed by Teddy).
 *                     This is used to resolve ambiguity and check constraints.
 * @param graph        Pointer to an iynux_execution_graph_t that will be filled.
 *                     The caller must free the strings in the events array and the array itself.
 * @return 0 on success, -1 on failure.
 *         On failure, *graph will be set to an empty graph (NULL events, count 0).
 */
int iynux_compile_intent(const char *intent, const char *system_state, iynux_execution_graph_t *graph);

#ifdef __cplusplus
}
#endif

#endif /* ENSer_IYNUX_H */