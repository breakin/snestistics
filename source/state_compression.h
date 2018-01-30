#pragma once

// This file handles compression of state snapshots of the emulator
// Only used to validate our emulation against the emulator used
// Since it will be used by both snestistics and emulator it is written to be fairly standalone

#include <stdint.h>
#include <cstdio>

struct StateCompression;

StateCompression *state_compression_create(uint32_t state_size);
void state_compression_destroy(StateCompression *sc, FILE *output);

// Make this one instead and add helper to do the old stuff
// void state_compression_add(StateCompression &sc, const uint32_t num_blocks, const uint8_t* const blocks, const uint32_t *block_sizes, FILE *output);
void state_compression_add(StateCompression *sc, const uint8_t* const updated_state, FILE *output, const uint32_t compression_method);

void state_compression_validate(FILE *compressed, const uint32_t state_size, const uint8_t * const uncompressed, const uint32_t uncompressed_size, const uint32_t compression_method);
