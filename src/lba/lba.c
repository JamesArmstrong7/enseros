/* © 2026 Juan Sebastian Alarcón Alarcón (@JamesArmstrong7). Prohibida su
copia, ingeniería inversa o estudio funcional. Ley 23 de 1982. */

#include "enser/lba.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "enser/pipo.h"

static const char *log_file_path = NULL;
static FILE *log_fp = NULL;

int lba_init(const char *log_dir) {
    if (log_dir) {
        // Ensure log directory exists
        struct stat st = {0};
        if (stat(log_dir, &st) == -1) {
            if (mkdir(log_dir, 0755) == -1) {
                return -1;
            }
        }
        // Construct log file path
        size_t len = strlen(log_dir) + strlen("/lba.jsonl") + 1;
        char *path = malloc(len);
        if (!path) {
            return -1;
        }
        snprintf(path, len, "%s/lba.jsonl", log_dir);
        log_file_path = path;
    } else {
        // Default directory
        const char *def_dir = "./lba";
        struct stat st = {0};
        if (stat(def_dir, &st) == -1) {
            if (mkdir(def_dir, 0755) == -1) {
                return -1;
            }
        }
        size_t len = strlen(def_dir) + strlen("/lba.jsonl") + 1;
        char *path = malloc(len);
        if (!path) {
            return -1;
        }
        snprintf(path, len, "%s/lba.jsonl", def_dir);
        log_file_path = path;
    }

    // Open log file for appending (create if doesn't exist)
    log_fp = fopen(log_file_path, "a");
    if (!log_fp) {
        if (log_dir == NULL) {
            free((void *)log_file_path);
        }
        return -1;
    }

    return 0;
}

void lba_deinit(void) {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
    if (log_file_path) {
        free((void *)log_file_path);
        log_file_path = NULL;
    }
}

int lba_append(const lba_event_t *event) {
    if (!event || !event->event_id[0] || !event->event_type[0] || !event->data) {
        return -1;
    }
    if (!log_fp) {
        return -1;
    }

    // We will write a JSON line:
    // {"event_id":"...","event_type":"...","timestamp":...,"data":..., "version":%d}
    // We assume the data string is a valid JSON string.
    int ret = fprintf(log_fp,
                      "{\"event_id\":\"%s\",\"event_type\":\"%s\",\"timestamp\":%f,\"data\":%s,\"version\":%d}\n",
                      event->event_id,
                      event->event_type,
                      event->timestamp,
                      event->data,
                      event->version);
    if (ret < 0) {
        return -1;
    }
    if (fflush(log_fp) != 0) {
        return -1;
    }
    return 0;
}

lba_event_t **lba_replay(size_t *out_count) {
    if (!out_count) {
        return NULL;
    }
    *out_count = 0;

    if (!log_file_path) {
        return NULL;
    }

    FILE *fp = fopen(log_file_path, "r");
    if (!fp) {
        return NULL;
    }

    // We'll read line by line and parse each line as a JSON object.
    // Given the simplicity, we will use a fixed buffer size for each line.
    // In a production system, we would use a dynamic buffer or a JSON parser.
    // We assume each line is less than 4096 bytes.
    #define MAX_LINE 4096
    char line[MAX_LINE];
    size_t capacity = 16;
    size_t count = 0;
    lba_event_t **events = malloc(capacity * sizeof(lba_event_t *));
    if (!events) {
        fclose(fp);
        return NULL;
    }

    while (fgets(line, sizeof(line), fp)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = 0;

        // Parse the JSON line.
        // We expect: {"event_id":"...","event_type":"...","timestamp":...,"data":...,"version":...}
        // We will use a simple parsing strategy: look for the keys and extract the values.
        // This is not robust but works for the format we generate.
        char *event_id = NULL;
        char *event_type = NULL;
        double timestamp = 0.0;
        char *data = NULL;
        int version = 0;

        // Find event_id
        char *ptr = strstr(line, "\"event_id\":\"");
        if (ptr) {
            ptr += strlen("\"event_id\":\"");
            char *end = strchr(ptr, '"');
            if (end) {
                size_t len = end - ptr;
                if (len < sizeof(((lba_event_t *)0)->event_id)) {
                    event_id = malloc(len + 1);
                    if (event_id) {
                        memcpy(event_id, ptr, len);
                        event_id[len] = 0;
                    }
                }
            }
        }

        // Find event_type
        ptr = strstr(line, "\"event_type\":\"");
        if (ptr) {
            ptr += strlen("\"event_type\":\"");
            char *end = strchr(ptr, '"');
            if (end) {
                size_t len = end - ptr;
                if (len < sizeof(((lba_event_t *)0)->event_type)) {
                    event_type = malloc(len + 1);
                    if (event_type) {
                        memcpy(event_type, ptr, len);
                        event_type[len] = 0;
                    }
                }
            }
        }

        // Find timestamp
        ptr = strstr(line, "\"timestamp\":");
        if (ptr) {
            ptr += strlen("\"timestamp\":");
            timestamp = strtod(ptr, NULL);
        }

        // Find data
        ptr = strstr(line, "\"data\":");
        if (ptr) {
            ptr += strlen("\"data\":");
            // The data is a JSON string, so it starts with a quote and ends with a quote.
            // We will copy the entire string including the quotes? Actually, we stored the data as a JSON string without the outer quotes?
            // In lba_append, we wrote: \"data\":%s, where %s is the data string. We assumed the data string is a JSON string, so it already has quotes if it is a string, or is a number, object, etc.
            // To keep it simple, we will copy the value as is until we hit a comma or brace.
            // We'll look for the next comma or '}' that is not inside a string.
            // Given the complexity, and since we are in a controlled environment, we will assume the data does not contain commas or braces unless escaped.
            // We will use a simple method: find the next comma or '}' that is preceded by an even number of backslashes? Too complex.
            // Instead, we will store the data as a string and in the replay, we will copy the substring from ptr to the next comma or '}' that is not inside quotes.
            // We'll implement a simple state machine to track if we are inside a string.
            // Given the time, we will assume the data is a simple string without commas or braces or quotes inside.
            // We will look for the next comma or '}'.
            char *end = ptr;
            int in_string = 0;
            while (*end && *end != '\n') {
                if (*end == '\"' && (end == ptr || *(end-1) != '\\')) {
                    in_string = !in_string;
                }
                if (!in_string && (*end == ',' || *end == '}')) {
                    break;
                }
                end++;
            }
            if (*end == ',' || *end == '}') {
                size_t len = end - ptr;
                data = malloc(len + 1);
                if (data) {
                    memcpy(data, ptr, len);
                    data[len] = 0;
                }
            }
        }

        // Find version
        ptr = strstr(line, "\"version\":");
        if (ptr) {
            ptr += strlen("\"version\":");
            version = atoi(ptr);
        }

        // If we have all fields, create the event.
        if (event_id && event_type && data) {
            lba_event_t *event = malloc(sizeof(lba_event_t));
            if (event) {
                strncpy(event->event_id, event_id, sizeof(event->event_id)-1);
                event->event_id[sizeof(event->event_id)-1] = 0;
                strncpy(event->event_type, event_type, sizeof(event->event_type)-1);
                event->event_type[sizeof(event->event_type)-1] = 0;
                event->timestamp = timestamp;
                event->data = data;
                event->version = version;
                if (count >= capacity) {
                    capacity *= 2;
                    lba_event_t **new_events = realloc(events, capacity * sizeof(lba_event_t *));
                    if (!new_events) {
                        // Free what we have so far
                        for (size_t i = 0; i < count; i++) {
                            lba_event_free(events[i]);
                        }
                        free(events);
                        events = NULL;
                        break;
                    }
                    events = new_events;
                }
                events[count] = event;
                count++;
            } else {
                // Free the strings we allocated for this event
                free(event_id);
                free(event_type);
                free(data);
            }
        } else {
            // Free any allocated strings
            free(event_id);
            free(event_type);
            free(data);
        }
    }

    fclose(fp);
    *out_count = count;
    return events;
}

lba_event_t **lba_get_events_by_entity(const char *entity_id, size_t *out_count) {
    if (!entity_id || !out_count) {
        return NULL;
    }

    // We can reuse lba_replay and then filter, but that would be inefficient.
    // Instead, we will replay and filter on the fly.
    if (!log_file_path) {
        return NULL;
    }

    FILE *fp = fopen(log_file_path, "r");
    if (!fp) {
        return NULL;
    }

    #define MAX_LINE 4096
    char line[MAX_LINE];
    size_t capacity = 16;
    size_t count = 0;
    lba_event_t **events = malloc(capacity * sizeof(lba_event_t *));
    if (!events) {
        fclose(fp);
        return NULL;
    }

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;

        // We need to check if the event_id starts with the given entity_id and an underscore.
        // The event_id format is: {entity_id}_{timestamp_millis}
        // So we check if the line contains "\"event_id\":\"{entity_id}_"
        char prefix[256];
        snprintf(prefix, sizeof(prefix), "\"event_id\":\"%s_", entity_id);
        if (strncmp(line, prefix, strlen(prefix)) != 0) {
            // Not an event for this entity
            continue;
        }

        // Now parse the line similarly to lba_replay.
        char *event_id = NULL;
        char *event_type = NULL;
        double timestamp = 0.0;
        char *data = NULL;
        int version = 0;

        // Find event_id
        char *ptr = strstr(line, "\"event_id\":\"");
        if (ptr) {
            ptr += strlen("\"event_id\":\"");
            char *end = strchr(ptr, '"');
            if (end) {
                size_t len = end - ptr;
                if (len < sizeof(((lba_event_t *)0)->event_id)) {
                    event_id = malloc(len + 1);
                    if (event_id) {
                        memcpy(event_id, ptr, len);
                        event_id[len] = 0;
                    }
                }
            }
        }

        // Find event_type
        ptr = strstr(line, "\"event_type\":\"");
        if (ptr) {
            ptr += strlen("\"event_type\":\"");
            char *end = strchr(ptr, '"');
            if (end) {
                size_t len = end - ptr;
                if (len < sizeof(((lba_event_t *)0)->event_type)) {
                    event_type = malloc(len + 1);
                    if (event_type) {
                        memcpy(event_type, ptr, len);
                        event_type[len] = 0;
                    }
                }
            }
        }

        // Find timestamp
        ptr = strstr(line, "\"timestamp\":");
        if (ptr) {
            ptr += strlen("\"timestamp\":");
            timestamp = strtod(ptr, NULL);
        }

        // Find data
        ptr = strstr(line, "\"data\":");
        if (ptr) {
            ptr += strlen("\"data\":");
            char *end = ptr;
            int in_string = 0;
            while (*end && *end != '\n') {
                if (*end == '\"' && (end == ptr || *(end-1) != '\\')) {
                    in_string = !in_string;
                }
                if (!in_string && (*end == ',' || *end == '}')) {
                    break;
                }
                end++;
            }
            if (*end == ',' || *end == '}') {
                size_t len = end - ptr;
                data = malloc(len + 1);
                if (data) {
                    memcpy(data, ptr, len);
                    data[len] = 0;
                }
            }
        }

        // Find version
        ptr = strstr(line, "\"version\":");
        if (ptr) {
            ptr += strlen("\"version\":");
            version = atoi(ptr);
        }

        if (event_id && event_type && data) {
            lba_event_t *event = malloc(sizeof(lba_event_t));
            if (event) {
                strncpy(event->event_id, event_id, sizeof(event->event_id)-1);
                event->event_id[sizeof(event->event_id)-1] = 0;
                strncpy(event->event_type, event_type, sizeof(event->event_type)-1);
                event->event_type[sizeof(event->event_type)-1] = 0;
                event->timestamp = timestamp;
                event->data = data;
                event->version = version;
                if (count >= capacity) {
                    capacity *= 2;
                    lba_event_t **new_events = realloc(events, capacity * sizeof(lba_event_t *));
                    if (!new_events) {
                        for (size_t i = 0; i < count; i++) {
                            lba_event_free(events[i]);
                        }
                        free(events);
                        events = NULL;
                        break;
                    }
                    events = new_events;
                }
                events[count] = event;
                count++;
            } else {
                free(event_id);
                free(event_type);
                free(data);
            }
        } else {
            free(event_id);
            free(event_type);
            free(data);
        }
    }

    fclose(fp);
    *out_count = count;
    return events;
}

lba_event_t *lba_get_latest_event(const char *entity_id) {
    if (!entity_id) {
        return NULL;
    }

    // We will get all events for the entity and then pick the one with the highest timestamp.
    size_t count = 0;
    lba_event_t **events = lba_get_events_by_entity(entity_id, &count);
    if (!events || count == 0) {
        if (events) {
            free(events);
        }
        return NULL;
    }

    lba_event_t *latest = events[0];
    double latest_ts = latest->timestamp;
    for (size_t i = 1; i < count; i++) {
        if (events[i]->timestamp > latest_ts) {
            lba_event_free(latest);
            latest = events[i];
            latest_ts = events[i]->timestamp;
        } else {
            lba_event_free(events[i]);
        }
    }
    free(events);
    return latest;
}

void lba_event_free(lba_event_t *event) {
    if (!event) {
        return;
    }
    free(event->data);
    free(event);
}

// Forward declarations for PIPO handlers
static int lba_pipo_handle_local(const void *request, void **response);
static int lba_pipo_escalate(const void *request);
static int lba_pipo_handle_daemon_response(const void *daemon_response, void **response);

// PIPO interface for LBA module
static const pipo_interface_t lba_pipo_interface = {
    .handle_local = lba_pipo_handle_local,
    .escalate = lba_pipo_escalate,
    .handle_daemon_response = lba_pipo_handle_daemon_response
};

static int lba_pipo_handle_local(const void *request, void **response) {
    if (!request || !response) {
        return -1;
    }
    const lba_event_t *event = request;
    lba_event_t *event_copy = malloc(sizeof(lba_event_t));
    if (!event_copy) {
        return -1;
    }
    // Initialize the copy
    event_copy->event_id[0] = 0;
    event_copy->event_type[0] = 0;
    event_copy->timestamp = event->timestamp;
    event_copy->data = NULL;
    event_copy->version = event->version;

    // Copy event_id (array, so always copy)
    strncpy(event_copy->event_id, event->event_id, sizeof(event_copy->event_id)-1);
    event_copy->event_id[sizeof(event_copy->event_id)-1] = 0;

    // Copy event_type (array)
    strncpy(event_copy->event_type, event->event_type, sizeof(event_copy->event_type)-1);
    event_copy->event_type[sizeof(event_copy->event_type)-1] = 0;

    // Copy data if present
    if (event->data) {
        event_copy->data = strdup(event->data);
        if (!event_copy->data) {
            free(event_copy);
            return -1;
        }
    }

    int ret = lba_append(event_copy);
    // Clean up the copy's data (since lba_append makes its own copy)
    free(event_copy->data);
    free(event_copy);
    if (ret != 0) {
        int *fail = malloc(sizeof(int));
        if (!fail) return -1;
        *fail = -1;
        *response = fail;
        return -1;
    }
    int *success = malloc(sizeof(int));
    if (!success) return -1;
    *success = 0;
    *response = success;
    return 0;
}

static int lba_pipo_escalate(const void *request) {
    (void)request;
    return -1; // Not implemented; all handling done locally for now
}

static int lba_pipo_handle_daemon_response(const void *daemon_response, void **response) {
    (void)daemon_response;
    (void)response;
    return -1; // Not implemented
}

int lba_pipo_init(const char *log_dir) {
    int ret = lba_init(log_dir);
    if (ret != 0) {
        return ret;
    }
    return pipo_init(&lba_pipo_interface);
}

void lba_pipo_deinit(void) {
    pipo_deinit();
    lba_deinit();
}
