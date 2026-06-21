#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>

#include "enser/causal_sequencer.h"

static const char *state_dir = "./enser_state";
static const char *state_file = "./enser_state/last_seq";

static void ensure_state_dir(void) {
    struct stat st = {0};
    if (stat(state_dir, &st) == -1) {
        mkdir(state_dir, 0755);
    }
}

uint64_t caser_get_next(void) {
    ensure_state_dir();

    FILE *fp = fopen(state_file, "r");
    uint64_t last_seq = 0;
    if (fp) {
        if (fscanf(fp, "%" SCNu64, &last_seq) != 1) {
            last_seq = 0;
        }
        fclose(fp);
    }

    uint64_t next_seq = last_seq + 1;

    // Atomically write the new sequence number
    char temp_file[256];
    snprintf(temp_file, sizeof(temp_file), "%s.tmp", state_file);
    FILE *tmp_fp = fopen(temp_file, "w");
    if (tmp_fp) {
        fprintf(tmp_fp, "%" PRIu64 "\n", next_seq);
        fflush(tmp_fp);
        fclose(tmp_fp);
        rename(temp_file, state_file);
    }

    return next_seq;
}

void caser_reset(void) {
    ensure_state_dir();

    FILE *fp = fopen(state_file, "w");
    if (fp) {
        fprintf(fp, "0\n");
        fclose(fp);
    }
}