#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "enser/encoder.h"
#include "enser/decoder.h"
#include "enser/causal_sequencer.h"
#include "enser/persistence_log.h"
#include "enser/hash.h"
#include "enser/hex.h"

static void reset_test_state(void) {
    // Reset causal sequencer
    // Remove storage directory
    system("rm -rf ./enser_storage ./enser_state");
}

int main(void) {
    reset_test_state();

    printf("=== Test Log Start ===\n");

    // Test causal sequencer
    printf("Testing causal sequencer...\n");
    uint64_t seq1 = caser_get_next();
    uint64_t seq2 = caser_get_next();
    if (seq2 != seq1 + 1) {
        fprintf(stderr, "FAIL: causal sequencer not monotonic: %" PRIu64 ", %" PRIu64 "\n", seq1, seq2);
        return 1;
    }
    printf("  caser_get_next: %" PRIu64 ", %" PRIu64 " (OK)\n", seq1, seq2);

    // Test persistence log init
    printf("Testing persistence log init...\n");
    if (plog_init("./enser_storage") != 0) {
        fprintf(stderr, "FAIL: plog_init failed\n");
        return 1;
    }
    printf("  plog_init: OK\n");

    // Test appending and reading an event
    printf("Testing event append and read...\n");
    const char *payload = "hello enser log";
    uint8_t *buffer = NULL;
    size_t size = 0;

    // Build an event using encoder (which will get a causal seq from caser_get_next)
    if (encr_build(
            NULL,
            0,
            (const uint8_t *)payload,
            strlen(payload),
            &buffer,
            &size) != 0) {
        fprintf(stderr, "FAIL: encr_build failed\n");
        return 1;
    }
    printf("  Built event, size: %zu bytes\n", size);

    // Validate the event
    if (encr_validate(buffer, size) != 0) {
        fprintf(stderr, "FAIL: encr_validate failed\n");
        free(buffer);
        return 1;
    }
    printf("  encr_validate: OK\n");

    // Compute hash
    uint8_t hash[32];
    if (encr_hash(buffer, size, hash) != 0) {
        fprintf(stderr, "FAIL: encr_hash failed\n");
        free(buffer);
        return 1;
    }
    printf("  encr_hash: OK\n");

    // Append to log
    if (plog_append(buffer, size) != 0) {
        fprintf(stderr, "FAIL: plog_append failed\n");
        free(buffer);
        return 1;
    }
    printf("  plog_append: OK\n");

    free(buffer);

    // Reset log reading
    plog_reset_read();

    // Read back the event
    uint8_t *read_buf = NULL;
    size_t read_size = 0;
    if (plog_read_next(&read_buf, &read_size) != 0) {
        fprintf(stderr, "FAIL: plog_read_next failed\n");
        return 1;
    }
    printf("  plog_read_next: read %zu bytes\n", read_size);

    // Validate the read event
    if (encr_validate(read_buf, read_size) != 0) {
        fprintf(stderr, "FAIL: read event validation failed\n");
        free(read_buf);
        return 1;
    }
    printf("  read event validation: OK\n");

    // Decode the event
    encr_object_t obj;
    if (encr_decoder(read_buf, read_size, &obj) != 0) {
        fprintf(stderr, "FAIL: encr_decoder failed\n");
        free(read_buf);
        return 1;
    }
    printf("  encr_decoder: OK\n");

    // Check payload
    if (read_size != sizeof(encr_header_t) + strlen(payload)) {
        fprintf(stderr, "FAIL: read event size mismatch\n");
        encr_object_free(&obj);
        free(read_buf);
        return 1;
    }
    if (memcmp(obj.payload, payload, strlen(payload)) != 0) {
        fprintf(stderr, "FAIL: read payload mismatch\n");
        encr_object_free(&obj);
        free(read_buf);
        return 1;
    }
    printf("  read payload matches: OK\n");

    // Check causal seq in the header (should be seq2? Actually, we built the event after two caser_get_next calls?)
    // Let's think: we called caser_get_next twice: seq1 and seq2.
    // Then we built the event, which inside encr_build calls caser_get_next again -> so it should be seq2+1.
    uint64_t expected_seq = seq2 + 1;
    uint64_t actual_seq = obj.header.causal_seq;
    if (actual_seq != expected_seq) {
        fprintf(stderr, "FAIL: causal seq mismatch: expected %" PRIu64 ", got %" PRIu64 "\n",
                expected_seq, actual_seq);
        encr_object_free(&obj);
        free(read_buf);
        return 1;
    }
    printf("  causal seq matches: %" PRIu64 " (OK)\n", actual_seq);

    encr_object_free(&obj);
    free(read_buf);

    // Test reading again should return EOF
    printf("Testing EOF after read...\n");
    if (plog_read_next(&read_buf, &read_size) == 0) {
        fprintf(stderr, "FAIL: expected EOF but got data\n");
        free(read_buf);
        return 1;
    }
    printf("  plog_read_next: EOF (OK)\n");

    // Test re-init of causal sequencer after reboot simulation
    printf("Testing causal sequencer persistence...\n");
    plog_deinit();
    // We'll re-init the log and then get the next seq, which should be the next after the last one we used.
    // But note: we reset the caser above, which is not what happens in a reboot.
    // In a reboot, the caser would read the last seq from the state file.
    // So we should not reset the caser here.

    // Let's redo: we have already appended one event, which used one causal seq (seq2+1).
    // The state file should have that value.

    // We'll re-init the log and caser (by just calling plog_init which doesn't touch caser) and then get next seq.

    // First, re-init log
    if (plog_init("./enser_storage") != 0) {
        fprintf(stderr, "FAIL: plog_init second time failed\n");
        return 1;
    }

    // Get next causal seq
    uint64_t seq3 = caser_get_next();
    // We expect seq3 to be (seq2+1) + 1 = seq2 + 2
    if (seq3 != seq2 + 2) {
        fprintf(stderr, "FAIL: caser_get_next after reboot: expected %" PRIu64 ", got %" PRIu64 "\n",
                seq2 + 2, seq3);
        return 1;
    }
    printf("  caser_get_next after reboot: %" PRIu64 " (expected %" PRIu64 ") (OK)\n", seq3, seq2 + 2);

    // Build another event and append
    const char *payload2 = "second event";
    uint8_t *buffer2 = NULL;
    size_t size2 = 0;
    if (encr_build(
            NULL,
            0,
            (const uint8_t *)payload2,
            strlen(payload2),
            &buffer2,
            &size2) != 0) {
        fprintf(stderr, "FAIL: encr_build second event failed\n");
        return 1;
    }
    if (plog_append(buffer2, size2) != 0) {
        fprintf(stderr, "FAIL: plog_append second event failed\n");
        free(buffer2);
        return 1;
    }
    printf("  Second event appended (OK)\n");
    free(buffer2);

    // Read both events and verify
    plog_reset_read();

    // First event
    uint8_t *read_buf1 = NULL;
    size_t read_size1 = 0;
    if (plog_read_next(&read_buf1, &read_size1) != 0) {
        fprintf(stderr, "FAIL: plog_read_next first event (second time) failed\n");
        return 1;
    }
    if (encr_validate(read_buf1, read_size1) != 0) {
        fprintf(stderr, "FAIL: first event validation failed\n");
        free(read_buf1);
        return 1;
    }
    encr_object_t obj1;
    if (encr_decoder(read_buf1, read_size1, &obj1) != 0) {
        fprintf(stderr, "FAIL: decod first event failed\n");
        free(read_buf1);
        return 1;
    }
    // Check causal seq of first event should be seq2+1
    uint64_t actual_seq1 = obj1.header.causal_seq;
    if (actual_seq1 != seq2 + 1) {
        fprintf(stderr, "FAIL: first event causal seq mismatch: expected %" PRIu64 ", got %" PRIu64 "\n",
                seq2 + 1, actual_seq1);
        encr_object_free(&obj1);
        free(read_buf1);
        return 1;
    }
    encr_object_free(&obj1);
    free(read_buf1);

    // Second event
    uint8_t *read_buf2 = NULL;
    size_t read_size2 = 0;
    if (plog_read_next(&read_buf2, &read_size2) != 0) {
        fprintf(stderr, "FAIL: plog_read_next second event failed\n");
        return 1;
    }
    if (encr_validate(read_buf2, read_size2) != 0) {
        fprintf(stderr, "FAIL: second event validation failed\n");
        free(read_buf2);
        return 1;
    }
    encr_object_t obj2;
    if (encr_decoder(read_buf2, read_size2, &obj2) != 0) {
        fprintf(stderr, "FAIL: decod second event failed\n");
        free(read_buf2);
        return 1;
    }
    // Check causal seq of second event should be seq3
    uint64_t actual_seq2 = obj2.header.causal_seq;
    if (actual_seq2 != seq3 + 1) {
        fprintf(stderr, "FAIL: second event causal seq mismatch: expected %" PRIu64 ", got %" PRIu64 "\n",
                seq3 + 1, actual_seq2);
        encr_object_free(&obj2);
        free(read_buf2);
        return 1;
    }
    encr_object_free(&obj2);
    free(read_buf2);

    // Check EOF
    if (plog_read_next(&read_buf2, &read_size2) == 0) {
        fprintf(stderr, "FAIL: expected EOF after second event\n");
        free(read_buf2);
        return 1;
    }

    printf("=== Test Log Passed ===\n");
    reset_test_state();
    return 0;
}