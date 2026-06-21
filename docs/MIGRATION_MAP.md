# ENSER Migration Map

## Batch 1: Cleanup Duplicate Test Files

### Action: DELETE
- `tests/test_create2.c`
- `tests/test_create3.c`
- `tests/test_create4.c`
- `tests/test_create5.c`

### Reason:
These files are duplicate implementations of `test_create.c` causing symbol collision during linking.
They are not part of the canonical ENSER test suite and introduce unnecessary complexity.

### Files to Keep:
- `tests/test_create.c` (primary event creation test)
- `tests/test_parse.c` (event parsing test)
- `tests/test_view.c` (event viewing/decoding test)

### Note:
Do not expand the CLI command surface in `tests/main.c` for these legacy variants.
The current test binary supports only `create`, `parse`, and `view` subcommands.
