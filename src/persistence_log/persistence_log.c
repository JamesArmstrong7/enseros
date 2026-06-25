/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "enser/persistence_log.h"

static const char *log_dir = NULL;
static const char *log_file_path = "./enser_storage/events.log";
static FILE *log_fp = NULL;
static FILE *read_fp = NULL;

int plog_init(const char *dir) {
    if (dir) {
        log_dir = dir;
        // construct log file path
        char *path = malloc(strlen(dir) + strlen("/events.log") + 1);
        if (!path) return -1;
        sprintf(path, "%s/events.log", dir);
        log_file_path = path;
    }

    // Ensure log directory exists
    if (log_dir) {
        struct stat st = {0};
        if (stat(log_dir, &st) == -1) {
            if (mkdir(log_dir, 0755) == -1) {
                free((void *)log_file_path);
                return -1;
            }
        }
    } else {
        // default directory
        struct stat st = {0};
        if (stat("./enser_storage", &st) == -1) {
            if (mkdir("./enser_storage", 0755) == -1) {
                return -1;
            }
        }
    }

    // Open log file for appending
    log_fp = fopen(log_file_path, "ab");
    if (!log_fp) {
        if (log_dir) free((void *)log_file_path);
        return -1;
    }

    // Open for reading (we'll open it later in plog_reset_read or on first read)
    read_fp = NULL;

    return 0;
}

void plog_deinit(void) {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    if (read_fp) {
        fclose(read_fp);
        read_fp = NULL;
    }
    if (log_dir && log_file_path) {
        free((void *)log_file_path);
        log_file_path = NULL;
        log_dir = NULL;
    }
}

int plog_append(const uint8_t *event, size_t event_size) {
    if (!log_fp || !event || event_size == 0) {
        return -1;
    }

    // Write the size as a big-endian uint32_t
    uint32_t size_be = htonl((uint32_t)event_size);
    size_t written = fwrite(&size_be, 1, sizeof(size_be), log_fp);
    if (written != sizeof(size_be)) {
        return -1;
    }

    // Write the event data
    written = fwrite(event, 1, event_size, log_fp);
    if (written != event_size) {
        return -1;
    }

    // Flush to ensure data is written
    if (fflush(log_fp) != 0) {
        return -1;
    }

    return 0;
}

int plog_read_next(uint8_t **out_event, size_t *out_size) {
    if (!out_event || !out_size) {
        return -1;
    }

    if (!read_fp) {
        // Open the log file for reading from the beginning
        read_fp = fopen(log_file_path, "rb");
        if (!read_fp) {
            return -1;
        }
    }

    // Read the size (4 bytes, big-endian)
    uint32_t size_be;
    size_t read_size = fread(&size_be, 1, sizeof(size_be), read_fp);
    if (read_size == 0) {
        // End of file
        return -1;
    }
    if (read_size != sizeof(size_be)) {
        // Incomplete size field
        return -1;
    }
    uint32_t event_size = ntohl(size_be);
    if (event_size == 0) {
        // Zero-sized event? We'll treat as invalid.
        return -1;
    }

    // Allocate buffer for the event
    uint8_t *event_buf = malloc(event_size);
    if (!event_buf) {
        return -1;
    }

    // Read the event data
    read_size = fread(event_buf, 1, event_size, read_fp);
    if (read_size != event_size) {
        free(event_buf);
        return -1;
    }

    *out_event = event_buf;
    *out_size = event_size;

    return 0;
}

void plog_reset_read(void) {
    if (read_fp) {
        fclose(read_fp);
        read_fp = NULL;
    }
}