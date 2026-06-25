/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

/*======================================================================
 *  PIPO (Micro-Agent Layer) – Ephemeral IPC Instance Model
 *  Zero‑copy payload handling, 2‑second GC window, non‑blocking loop
 *====================================================================*/

#include "enser/pipo.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>

/* ------------------------------------------------------------------
 *  Micro‑Agent state (unchanged from the reviewed version)
 * ------------------------------------------------------------------ */
static const pipo_interface_t *module_interface = NULL;
static void   *daemon_response_data = NULL;
static int     daemon_response_available = 0;

/* ------------------------------------------------------------------
 *  Ephemeral IPC instance pool (static, fits the 64 MB budget)
 * ------------------------------------------------------------------ */
static pipo_ipc_instance_t ipc_instances[PIPO_IPC_MAX_INSTANCES];
static int     daemon_socket = -1;
static bool    daemon_running = false;
static time_t  last_gc_time = 0;

/* Forward declarations for daemon helpers */
static int daemon_accept_client(int listen_fd);
static ssize_t daemon_read_message(int fd, void *buf, size_t len);
static ssize_t daemon_write_message(int fd, const void *buf, size_t len);

/* ------------------------------------------------------------------
 *  Micro‑Agent API (pipo_init, pipo_deinit, pipo_handle_request)
 * ------------------------------------------------------------------ */
int pipo_init(const pipo_interface_t *interface)
{
    if (!interface || !interface->handle_local || !interface->escalate ||
        !interface->handle_daemon_response)
        return -1;

    module_interface = interface;
    daemon_response_data = NULL;
    daemon_response_available = 0;
    return 0;
}

void pipo_deinit(void)
{
    module_interface = NULL;
    daemon_response_data = NULL;
    daemon_response_available = 0;
}

int pipo_handle_request(const void *request, void **response)
{
    if (!module_interface || !response)
        return -1;

    void *local_response = NULL;
    int local_ret = module_interface->handle_local(request, &local_response);
    if (local_ret == 0) {
        *response = local_response;
        return 0;
    }

    if (module_interface->escalate(request) != 0) {
        if (local_response)
            free(local_response);
        return -1;
    }

    if (!daemon_response_available) {
        if (local_response)
            free(local_response);
        return -1;
    }

    void *final_response = NULL;
    int daemon_ret = module_interface->handle_daemon_response(
            daemon_response_data, &final_response);
    daemon_response_data = NULL;
    daemon_response_available = 0;

    if (daemon_ret != 0) {
        if (local_response)
            free(local_response);
        return -1;
    }

    *response = final_response;
    return 0;
}

/* Test‑only helper – in a real system the daemon would set the response
 * via some IPC mechanism. */
void pipo_set_daemon_response(void *response_data)
{
    daemon_response_data = response_data;
    daemon_response_available = 1;
}

/* ------------------------------------------------------------------
 *  Ephemeral IPC Instance Management
 * ------------------------------------------------------------------ */
int pipo_ipc_instance_acquire(const pipo_hash_t hash, uint32_t *out_instance)
{
    if (!out_instance)
        return -1;

    /* Try instance 0 first (permanent), then 1..MAX‑1 */
    for (uint32_t i = 0; i < PIPO_IPC_MAX_INSTANCES; ++i) {
        if (!ipc_instances[i].in_use) {
            ipc_instances[i].in_use = true;
            ipc_instances[i].last_used = time(NULL);
            memcpy(ipc_instances[i].hash, hash, sizeof(pipo_hash_t));
            *out_instance = i;
            return 0;
        }
    }
    return -1; /* no free instances */
}

int pipo_ipc_instance_release(uint32_t instance_id)
{
    if (instance_id >= PIPO_IPC_MAX_INSTANCES)
        return -1;
    if (!ipc_instances[instance_id].in_use)
        return -1; /* already idle */

    ipc_instances[instance_id].in_use = false;
    ipc_instances[instance_id].last_used = 0;
    memset(ipc_instances[instance_id].hash, 0, sizeof(pipo_hash_t));
    return 0;
}

void pipo_ipc_instance_gc(void)
{
    time_t now = time(NULL);
    for (int i = 0; i < PIPO_IPC_MAX_INSTANCES; ++i) {
        if (ipc_instances[i].instance_id > 0 && !ipc_instances[i].in_use &&
            (now - ipc_instances[i].last_used) >= 2) {
            ipc_instances[i].in_use = false;
            ipc_instances[i].last_used = 0;
            memset(ipc_instances[i].hash, 0, sizeof(pipo_hash_t));
        }
    }
    last_gc_time = time(NULL);
}

/* ------------------------------------------------------------------
 *  Daemon Core – non‑blocking loop with 2‑second GC guarantee
 * ------------------------------------------------------------------ */
int pipo_daemon_init(const pipo_daemon_config_t *cfg)
{
    if (!cfg || !cfg->socket_path)
        return -1;

    /* Zero‑initialize instance pool */
    for (int i = 0; i < PIPO_IPC_MAX_INSTANCES; ++i) {
        ipc_instances[i].instance_id = i;
        ipc_instances[i].in_use = false;
        ipc_instances[i].last_used = 0;
        memset(ipc_instances[i].hash, 0, sizeof(pipo_hash_t));
    }

    /* Create UNIX domain socket (non‑blocking) */
    daemon_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (daemon_socket < 0)
        return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    if (cfg->use_abstract) {
        /* Abstract namespace: sun_path[0] = '\0', then name */
        addr.sun_path[0] = '\0';
        if (strlen(cfg->socket_path) + 1 >= sizeof(addr.sun_path)) {
            close(daemon_socket);
            daemon_socket = -1;
            return -1;
        }
        memcpy(&addr.sun_path[1], cfg->socket_path,
               strlen(cfg->socket_path) + 1);
    } else {
        if (strlen(cfg->socket_path) >= sizeof(addr.sun_path)) {
            close(daemon_socket);
            daemon_socket = -1;
            return -1;
        }
        strcpy(addr.sun_path, cfg->socket_path);
    }

    if (bind(daemon_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(daemon_socket);
        daemon_socket = -1;
        return -1;
    }
    if (listen(daemon_socket, 5) < 0) {
        close(daemon_socket);
        daemon_socket = -1;
        return -1;
    }

    daemon_running = true;
    last_gc_time = time(NULL);
    return 0;
}

void pipo_daemon_deinit(void)
{
    daemon_running = false;
    if (daemon_socket >= 0) {
        close(daemon_socket);
        daemon_socket = -1;
    }
    /* Static instance pool needs no explicit free */
}

/* Helper: accept a client (non‑blocking listen socket) */
static int daemon_accept_client(int listen_fd)
{
    struct sockaddr_un addr;
    socklen_t addr_len = sizeof(addr);
    int client_fd = accept(listen_fd, (struct sockaddr *)&addr, &addr_len);
    if (client_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;           /* no pending connection */
        return -1;
    }
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    return client_fd;
}

/* Helper: read exactly 'len' bytes (assumes non‑blocking fd; caller uses poll) */
static ssize_t daemon_read_message(int fd, void *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t r = read(fd, (char *)buf + total, len - total);
        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return -1;    /* try again later */
            return -1;
        }
        if (r == 0)           /* EOF */
            return total;
        total += r;
    }
    return total;
}

/* Helper: write exactly 'len' bytes (non‑blocking fd) */
static ssize_t daemon_write_message(int fd, const void *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t w = write(fd, (const char *)buf + total, len - total);
        if (w < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return -1;
            return -1;
        }
        total += w;
    }
    return total;
}

/* ------------------------------------------------------------------
 *  Zero‑copy client handler
 *
 *  Expected wire format (packed, no padding):
 *      struct {
 *          pipo_hash_t source_hash;   // 32 bytes
 *          uintptr_t   payload_address; // address of payload in shared memory
 *          uint32_t    payload_size;   // size in bytes
 *      };
 *
 *  The daemon:
 *      1. Reads the structure.
 *      2. Calls pi(source_hash, &instance) to acquire an ephemeral IPC instance.
 *      3. Invokes the module's privileged path via its public API
 *         (module_interface->handle_local) passing a pointer to the payload.
 *      4. Sends a one‑byte acknowledgment back to the client.
 *      5. Releases the instance with po(instance_id).
 *      6. Closes the socket (connection is ephemeral).
 * ------------------------------------------------------------------ */
static void daemon_handle_client(int client_fd)
{
    /* ---- 1. Receive the zero‑copy descriptor --------------------- */
    typedef struct {
        pipo_hash_t source_hash;
        uintptr_t   payload_address;
        uint32_t    payload_size;
    } __attribute__((packed)) pipo_zc_msg_t;

    pipo_zc_msg_t msg;
    ssize_t n = daemon_read_message(client_fd, &msg, sizeof(msg));
    if (n != sizeof(msg)) {
        /* malformed or truncated message – just close */
        close(client_fd);
        return;
    }

    /* ---- 2. Acquire an ephemeral IPC instance ------------------- */
    uint32_t instance_id;
    if (pi(msg.source_hash, &instance_id) != 0) {
        /* No free instances – send NAK and close */
        uint8_t nak = 0;
        daemon_write_message(client_fd, &nak, 1);
        close(client_fd);
        return;
    }

    /* ---- 3. Privileged processing --------------------------------
     * We treat the payload address/size as the request data.
     * The module's handle_local is expected to interpret it
     * appropriately (e.g., cast to its own request struct).
     * ----------------------------------------------------------------- */
    void *local_response = NULL;
    int ret = module_interface->handle_local(&msg, &local_response);
    uint8_t ack = (ret == 0) ? 1 : 0;   /* 1 = success, 0 = failure */

    /* ---- 4. Return acknowledgment -------------------------------- */
    daemon_write_message(client_fd, &ack, 1);

    /* ---- 5. Clean up ------------------------------------------------ */
    if (local_response)
        free(local_response);   /* module allocated it */
    po(instance_id);            /* release instance for GC */
    close(client_fd);
}

/* ------------------------------------------------------------------
 *  Daemon main loop – poll with 2‑second timeout, GC on timeout
 *      OR if >=2 seconds have elapsed since last GC.
 * ------------------------------------------------------------------ */
int pipo_daemon_run(void)
{
    if (!daemon_running || daemon_socket < 0)
        return -1;

    while (daemon_running) {
        struct pollfd fds[1];
        fds[0].fd     = daemon_socket;
        fds[0].events = POLLIN;

        int ret = poll(fds, 1, 2000);   /* 2000 ms = 2 s */
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;                     /* unrecoverable error */
        }

        if (fds[0].revents & POLLIN) {
            int client_fd = daemon_accept_client(daemon_socket);
            if (client_fd >= 0) {
                daemon_handle_client(client_fd);
                /* client_fd closed inside daemon_handle_client */
            }
        }

        /* ---- Garbage Collector: run if poll timed out OR >=2s passed ---- */
        time_t now = time(NULL);
        if (ret == 0 || (now - last_gc_time) >= 2)
            pipo_ipc_instance_gc();
    }
    return 0;
}