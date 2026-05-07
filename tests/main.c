#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "enser/encoder.h"
#include "enser/storage.h"

void print_hash(uint8_t hash[32]) {
    for (int i = 0; i < 32; i++)
        printf("%02x", hash[i]);
    printf("\n");
}

int main(int argc, char **argv) {

    uint8_t *buffer = NULL;
    size_t size = 0;
    uint8_t hash[32];

    if (argc < 2) {
    printf("usage:\n");
    printf("  %s create\n", argv[0]);
    printf("  %s inspect <hash>\n", argv[0]);
    return 1;
    }
    if (strcmp(argv[1], "create") == 0) {

	if (encoder_buid(
	    1, 1, 1, 1,
	    NULL, 0,
	    NULL, 0,
	    &buffer, &size
	) != 0 {
	    printf("build error\n");
	    return 1;
	}

        printf("Event size: %zu bytes\n", size);

	if (encoder_validate(buffer, size) != 0) {
	printf("invalid event\n");
	return 1;
	}

	if (encoder_hash(buffer, size, hash) != 0) {
	printf("hash error\n");
	return 1;
	}

	printf("valid event\n");
	printf("Hash: ");
	print_hash(hash);

	// Parse

	/*
	encoder_view_t view;

	if (encoder_parse(buffer, size, &view) != 0) {
	printf("parse error\n");
	return 1;
	}

	printf("Parsed:\n");
	printf("  sys: %u\n", view.hdr->sys);
	printf("  id : %u\n", view.hdr->id);
	printf("  refs_count: %u\n", view.refs_count);
	printf("  payload_size: %u\n", view.payload_size);

	*/
	// store hash

	char hash_hex[65];

	for (int i = 0; i < 32; i++) {
	sprintf(
	    &hash_hex[i * 2],
	    "%02x",
	    hash[i]
	);
	}

	hash_hex[64] = '\0';

	if (
	enser_storage_write(
	    hash_hex,
	    buffer,
	    size
	) == 0
	)

	free(buffer);
	return 0;


    }
}
