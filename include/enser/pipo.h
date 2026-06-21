#ifndef ENSer_PIPO_H
#define ENSer_PIPO_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief The PIPO interface that a module must provide to its PIPO micro-agent.
 *        The PIPO micro-agent uses these functions to interact with the module.
 */
typedef struct {
    /**
     * @brief Handle a request locally (Fast Path).
     * @param request  The request data (owned by the caller).
     * @param response Pointer to a pointer that will receive the response data.
     *                 The PIPO micro-agent will allocate the response and the caller must free it.
     * @return 0 if the request was handled locally, -1 if escalation is needed.
     */
    int (*handle_local)(const void *request, void **response);

    /**
     * @brief Escalate a request to the PIPO Daemon.
     * @param request  The request data (owned by the caller).
     *                 The PIPO micro-agent expects that this function will send the request to the daemon
     *                 and arrange for a response to be delivered later via handle_daemon_response.
     * @return 0 if the escalation was initiated successfully, -1 on failure.
     */
    int (*escalate)(const void *request);

    /**
     * @brief Handle a response from the PIPO Daemon.
     * @param daemon_response  The response data from the daemon (owned by the daemon or the PIPO micro-agent).
     * @param response         Pointer to a pointer that will receive the final response data to return to the caller.
     *                         The PIPO micro-agent will allocate the response and the caller must free it.
     * @return 0 on success, -1 on failure.
     */
    int (*handle_daemon_response)(const void *daemon_response, void **response);
} pipo_interface_t;

/**
 * @brief Initialize the PIPO micro-agent for a module.
 * @param interface  The PIPO interface provided by the module (must not be NULL).
 * @return 0 on success, -1 on failure.
 */
int pipo_init(const pipo_interface_t *interface);

/**
 * @brief Deinitialize the PIPO micro-agent, releasing any resources.
 * @note  After calling this, the PIPO micro-agent should not be used until re-initialized.
 */
void pipo_deinit(void);

/**
 * @brief Handle an incoming request using the PIPO micro-agent.
 *        This function is the entry point for all requests to the module.
 * @param request  The request data (owned by the caller).
 * @param response Pointer to a pointer that will receive the response data.
 *                 The PIPO micro-agent will allocate the response and the caller must free it.
 * @return 0 on success (a response was generated), -1 on failure.
 */
int pipo_handle_request(const void *request, void **response);

#ifdef __cplusplus
}
#endif

#endif /* ENSer_PIPO_H */