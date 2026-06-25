/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include "enser/teddy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static char *lba_log_path = NULL;

int teddy_init(const char *lba_dir) {
    if (!lba_dir) {
        return -1;
    }
    // Free any existing path
    if (lba_log_path) {
        free(lba_log_path);
        lba_log_path = NULL;
    }
    // Construct the path to the lba.jsonl file
    size_t len = strlen(lba_dir) + strlen("/lba.jsonl") + 1;
    lba_log_path = malloc(len);
    if (!lba_log_path) {
        return -1;
    }
    snprintf(lba_log_path, len, "%s/lba.jsonl", lba_dir);
    // Check if the file exists and is readable
    if (access(lba_log_path, R_OK) == -1) {
        free(lba_log_path);
        lba_log_path = NULL;
        return -1;
    }
    return 0;
}

void teddy_deinit(void) {
    if (lba_log_path) {
        free(lba_log_path);
        lba_log_path = NULL;
    }
}

// Helper function to extract the data field from a JSON line.
// Returns a newly allocated string containing the data, or NULL if not found or on error.
// The caller must free the returned string.
static char *extract_data_from_line(const char *line) {
    if (!line) {
        return NULL;
    }
    // Look for the pattern: "data":"<value>"
    const char *pattern = "\"data\":\"";
    const char *pos = strstr(line, pattern);
    if (!pos) {
        return NULL;
    }
    pos += strlen(pattern);
    // Find the closing quote
    const char *end = strchr(pos, '\"');
    if (!end) {
        return NULL;
    }
    size_t len = end - pos;
    char *data = malloc(len + 1);
    if (!data) {
        return NULL;
    }
    memcpy(data, pos, len);
    data[len] = '\0';
    return data;
}

int teddy_replay_full(char **state) {
    if (!state || !lba_log_path) {
        return -1;
    }
    FILE *fp = fopen(lba_log_path, "r");
    if (!fp) {
        return -1;
    }
    char *full_state = NULL;
    size_t full_state_len = 0;
    size_t full_state_size = 0;
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;
        char *data = extract_data_from_line(line);
        if (data) {
            size_t data_len = strlen(data);
            // Resize full_state to accommodate the new data and a newline
            size_t new_len = full_state_len + data_len + 1; // +1 for newline
            if (new_len > full_state_size) {
                size_t new_size = (new_len * 2) + 256; // grow by factor of 2 plus some
                char *new_full_state = realloc(full_state, new_size);
                if (!new_full_state) {
                    free(data);
                    free(full_state);
                    fclose(fp);
                    return -1;
                }
                full_state = new_full_state;
                full_state_size = new_size;
            }
            // Append the data and a newline
            memcpy(&full_state[full_state_len], data, data_len);
            full_state_len += data_len;
            full_state[full_state_len] = '\n';
            full_state_len++;
            free(data);
        }
    }
    fclose(fp);
    if (full_state_len > 0 && full_state[full_state_len-1] == '\n') {
        full_state_len--; // remove the trailing newline
        full_state[full_state_len] = '\0';
    } else {
        // Ensure null termination
        if (full_state_len >= full_state_size) {
            // This should not happen because we grew the buffer
            free(full_state);
            return -1;
        }
        full_state[full_state_len] = '\0';
    }
    *state = full_state;
    return 0;
}

int teddy_replay_entity(const char *entity_id, char **state) {
    if (!entity_id || !state || !lba_log_path) {
        return -1;
    }
    if (entity_id[0] == '\0') {
        return -1;
    }
    FILE *fp = fopen(lba_log_path, "r");
    if (!fp) {
        return -1;
    }
    char *entity_state = NULL;
    size_t entity_state_len = 0;
    size_t entity_state_size = 0;
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char *data = extract_data_from_line(line);
        if (data) {
            // Check if the data string contains the entity_id
            if (strstr(data, entity_id) != NULL) {
                size_t data_len = strlen(data);
                size_t new_len = entity_state_len + data_len + 1; // +1 for newline
                if (new_len > entity_state_size) {
                    size_t new_size = (new_len * 2) + 256;
                    char *new_entity_state = realloc(entity_state, new_size);
                    if (!new_entity_state) {
                        free(data);
                        free(entity_state);
                        fclose(fp);
                        return -1;
                    }
                    entity_state = new_entity_state;
                    entity_state_size = new_size;
                }
                memcpy(&entity_state[entity_state_len], data, data_len);
                entity_state_len += data_len;
                entity_state[entity_state_len] = '\n';
                entity_state_len++;
            }
            free(data);
        }
    }
    fclose(fp);
    if (entity_state_len > 0 && entity_state[entity_state_len-1] == '\n') {
        entity_state_len--; // remove trailing newline
        entity_state[entity_state_len] = '\0';
    } else {
        if (entity_state_len >= entity_state_size) {
            free(entity_state);
            return -1;
        }
        entity_state[entity_state_len] = '\0';
    }
    *state = entity_state;
    return 0;
}

int teddy_replay_resource(const char *resource_id, char **state) {
    // For simplicity, we reuse the entity function, assuming resource_id can be treated like an entity_id.
    return teddy_replay_entity(resource_id, state);
}