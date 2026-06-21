#include "enser/pipo.h"
#include <stdlib.h>
#include <string.h>

static const pipo_interface_t *module_interface = NULL;
static void *daemon_response_data = NULL;
static int daemon_response_available = 0;

int pipo_init(const pipo_interface_t *interface) {
    if (!interface || !interface->handle_local || !interface->escalate ||
        !interface->handle_daemon_response) {
        return -1;
    }
    module_interface = interface;
    // Clear any stale daemon response
    daemon_response_data = NULL;
    daemon_response_available = 0;
    return 0;
}

void pipo_deinit(void) {
    module_interface = NULL;
    daemon_response_data = NULL;
    daemon_response_available = 0;
}

int pipo_handle_request(const void *request, void **response) {
    if (!module_interface || !response) {
        return -1;
    }

    void *local_response = NULL;
    int local_ret = module_interface->handle_local(request, &local_response);
    if (local_ret == 0) {
        *response = local_response;
        return 0;
    }
    // If handle_local returned -1, we need to escalate
    if (module_interface->escalate(request) != 0) {
        // Escalation failed
        if (local_response) {
            free(local_response);
        }
        return -1;
    }
    // Wait for daemon response (in our simulation, we assume it's already set)
    if (!daemon_response_available) {
        // In a real system, we would wait or block until a response arrives.
        // For this simulation, we treat missing daemon response as an error.
        if (local_response) {
            free(local_response);
        }
        return -1;
    }
    void *final_response = NULL;
    int daemon_ret = module_interface->handle_daemon_response(daemon_response_data, &final_response);
    // Clear the daemon response after use
    daemon_response_data = NULL;
    daemon_response_available = 0;
    if (daemon_ret != 0) {
        if (local_response) {
            free(local_response);
        }
        return -1;
    }
    *response = final_response;
    return 0;
}

// This function is intended to be used by tests to simulate the daemon response.
// In a real system, the PIPO Daemon would set this via some IPC mechanism.
void pipo_set_daemon_response(void *response_data) {
    daemon_response_data = response_data;
    daemon_response_available = 1;
}