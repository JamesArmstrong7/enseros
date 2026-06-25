/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include "enser/iyunux.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int iynux_init(void) {
    /* No state to initialize */
    return 0;
}

void iynux_deinit(void) {
    /* No state to free */
}

/**
 * @brief Helper function to duplicate a string array.
 * @param src Source array of strings.
 * @param count Number of strings.
 * @return A new array of duplicated strings, or NULL on failure.
 *         The caller must free each string and the array.
 */
static char **dup_string_array(const char * const *src, size_t count) {
    if (!src || count == 0) {
        return NULL;
    }
    char **dest = malloc(count * sizeof(char *));
    if (!dest) {
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        dest[i] = strdup(src[i]);
        if (!dest[i]) {
            /* Clean up */
            for (size_t j = 0; j < i; j++) {
                free(dest[j]);
            }
            free(dest);
            return NULL;
        }
    }
    return dest;
}

int iynux_compile_intent(const char *intent, const char *system_state, iynux_execution_graph_t *graph) {
    if (!intent || !graph) {
    (void)system_state;
        return -1;
    }

    /* Initialize output graph to empty */
    graph->events = NULL;
    graph->event_count = 0;

    /* Simple intent recognition: we match a few hardcoded intents.
     * In a real system, this would use NLP and consider system_state.
     */
    if (strcmp(intent, "quiero suscribirme a un comic") == 0 ||
        strcmp(intent, "I want to subscribe to a comic") == 0) {
        static const char *events[] = {
            "CREATE_SUBSCRIPTION",
            "LINK_USER",
            "INIT_BILLING"
        };
        graph->events = dup_string_array(events, sizeof(events)/sizeof(events[0]));
        if (!graph->events) {
            return -1;
        }
        graph->event_count = sizeof(events)/sizeof(events[0]);
        return 0;
    }

    if (strcmp(intent, "quiero acceder a contenido premium") == 0 ||
        strcmp(intent, "I want to access premium content") == 0) {
        static const char *events[] = {
            "REQUEST_ACCESS",
            "VALIDATE_SUBSCRIPTION",
            "GRANT_ACCESS"
        };
        graph->events = dup_string_array(events, sizeof(events)/sizeof(events[0]));
        if (!graph->events) {
            return -1;
        }
        graph->event_count = sizeof(events)/sizeof(events[0]);
        return 0;
    }

    if (strcmp(intent, "quiero cancelar mi suscripcion") == 0 ||
        strcmp(intent, "I want to cancel my subscription") == 0) {
        static const char *events[] = {
            "CANCEL_SUBSCRIPTION",
            "STOP_BILLING"
        };
        graph->events = dup_string_array(events, sizeof(events)/sizeof(events[0]));
        if (!graph->events) {
            return -1;
        }
        graph->event_count = sizeof(events)/sizeof(events[0]);
        return 0;
    }

    /* If we don't recognize the intent, return an empty graph (failure) */
    return -1;
}