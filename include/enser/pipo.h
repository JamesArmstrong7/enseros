/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#ifndef ENSer_PIPO_H
#define ENSer_PIPO_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

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

/*======================================================================
 * Ephemeral IPC Instance Model (2‑second windows)
 *====================================================================*/

/**
 * @brief Fixed‑size cryptographic identifier for a PIPO (SHA‑256).
 *        All instances are addressed as `<hash>_<instance_id>`.
 */
typedef uint8_t pipo_hash_t[32];

/**
 * @brief Runtime state of a single IPC instance.
 *        Instance _0 is permanent; instances >0 are created/destroyed
 *        on demand and reclaimed every 2 seconds if idle.
 */
typedef struct {
    pipo_hash_t      hash;          ///< SHA‑256 of the PIPO (zero‑filled if unregistered)
    uint32_t         instance_id;   ///< _0 = permanent; >0 = ephemeral
    bool             in_use;        ///< true while a transfer is active
    time_t           last_used;     ///< monotonic timestamp (seconds) of last activity
} pipo_ipc_instance_t;

/**
 * @brief Maximum number of concurrent ephemeral instances allowed.
 *        Chosen to fit comfortably inside the 64 MB per‑module budget.
 */
#define PIPO_IPC_MAX_INSTANCES  64

/**
 * @brief Acquire (or create) an IPC instance for the given hash.
 *        If hash_n is busy, the daemon will automatically try hash_n+1,
 *        hash_n+2, … up to PIPO_IPC_MAX_INSTANCES‑1.
 *        Instance _0 is always returned if it is free.
 *
 * @param[in]  hash          SHA‑256 identifier of the PIPO (zeroes if unregistered)
 * @param[out] out_instance  Filled with the instance_id that was granted
 * @return 0 on success, -1 on failure (no free instances)
 */
int pipo_ipc_instance_acquire(const pipo_hash_t hash, uint32_t *out_instance);

/**
 * @brief Release an IPC instance, marking it as idle.
 *        The instance will be reclaimed by the 2‑second garbage‑collector
 *        if it remains unused.
 *
 * @param[in]  instance_id   The identifier returned by pipo_ipc_instance_acquire
 * @return 0 on success, -1 on invalid instance_id
 */
int pipo_ipc_instance_release(uint32_t instance_id);

/**
 * @brief Garbage‑collector to be invoked every 2 seconds.
 *        Scans all instances; any instance_id > 0 that is !in_use and
 *        whose (now - last_used) >= 2 seconds is destroyed (reset to zero).
 *        This call must be timed by the CNUX metronome or a dedicated timer.
 */
void pipo_ipc_instance_gc(void);

/**
 * @brief Lookup an instance by its hash and instance_id.
 *        Returns NULL if not found or not currently allocated.
 *
 * @param[in]  hash          SHA‑256 identifier
 * @param[in]  instance_id   Desired instance (_0 … PIPO_IPC_MAX_INSTANCES‑1)
 * @return Pointer to the instance structure or NULL
 */
const pipo_ipc_instance_t *pipo_ipc_instance_lookup(const pipo_hash_t hash,
                                                    uint32_t instance_id);

/*======================================================================
 * Critical Module Micro-Interface (Injected Functions)
 *====================================================================*/

/**
 * @brief Pipe-In: Adquiere de forma transparente una instancia IPC libre para el
 * módulo crítico emisor e inicializa la transferencia por dirección (Zero-Copy).
 * @param[in]  source_hash El hash criptográfico del PIPO emisor.
 * @param[out] out_instance Puntero donde se almacenará el ID asignado dinámicamente.
 * @return 0 si el canal fue abierto con éxito, -1 si no hay instancias libres.
 */
int pi(const pipo_hash_t source_hash, uint32_t *out_instance);

/**
 * @brief Pipe-Out: Finaliza la transferencia del módulo crítico, marca la instancia
 * como ociosa para dejarla disponible y prepara el canal para el GC de 2 segundos.
 * @param[in]  instance_id El ID de la instancia que finalizó su transferencia.
 * @return 0 en éxito, -1 si el ID de instancia es inválido o no estaba activo.
 */
int po(uint32_t instance_id);

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