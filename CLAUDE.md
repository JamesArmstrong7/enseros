# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Development Commands

### Building
- `make` or `make all` - Compiles the test binary (`bin/test`)
- `make run` - Builds and runs all tests
- `make clean` - Removes object files and bin directory

### Running Tests
- `bin/test create` - Runs the event creation test
- `bin/test parse` - Runs the event parsing test
- `bin/test view <file>` - Views and decodes an event file (requires file path)

### Individual Test Source
Tests are located in `tests/`:
- `test_create.c` - Tests event building, validation, hashing, and storage
- `test_parse.c` - Tests event building and decoding
- `test_view.c` - Reads and displays event file contents
- `main.c` - Test dispatcher accepting commands: create, parse, view

## Architecture Overview

### Core Components
The ENSER system implements a content-addressable event store with the following layers:

#### Interface Layer (`include/enser/`)
Public header files defining the core API:
- `encoder.h`/`encr.h`: Event construction, validation, and hashing (ENCR format)
- `decoder.h`: Event decoding and object extraction
- `hash.h`: Cryptographic hashing (SHA-256 implementation)
- `hex.h`: Binary-to-hexadecimal conversion utilities
- `storage.h`: Content-addressed storage interface (write/read by hash)

#### Implementation Layer (`src/core/`)
Core functionality implementations:
- `encoder.c` - ENCR event building and validation
- `decoder.c` - Event decoding into structured objects
- `hash.c` - SHA-256 hashing implementation
- `storage.c` - File-based storage using hash-derived paths

### Event Format (ENCR)
Events follow a binary format with:
- Fixed header: magic ("ENCR"), version, refs_count, payload_size, timestamp
- 32-byte hash (SHA-256 of header+payload)
- Variable-length payload data
- Optional reference array (referenced by refs_count)

### Storage Mechanism
Events are stored using their SHA-256 hash as filename:
- `enser_storage_write()` writes events to `storage/<first_2_chars>/<remaining_hex>`
- Content-addressed deduplication (same hash = same event)
- Append-only semantics through immutable hash keys

### Data Flow (typical operation)
1. Build event: `encr_build(refs, refs_count, payload, payload_size, &buffer, &size)`
2. Validate: `encr_validate(buffer, size)`
3. Hash: `encr_hash(buffer, size, hash_output)` → 32-byte SHA-256
4. Store: `enser_storage_write(hex_hash, buffer, size)`
5. Retrieve: Read file → `enser_storage_path()` → `encr_decoder()` → access fields

### Dependencies
- POSIX.1-2008 (`-D_POSIX_C_SOURCE=200809L`)
- C11 standard (`-std=c11`)
- Standard C library only (no external dependencies)

### Notes
- The `src/lba/` directory referenced in Makefile does not currently exist
- Storage directory (`bin/storage/` after tests) holds content-addressed event files
- Tests demonstrate basic event lifecycle: build → validate → hash → store → retrieve → decode