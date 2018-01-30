#include "state_compression.h"
#include <cassert>
#include <string>
#include <algorithm>
#include "../deps/lz4/lib/lz4.h"

/*
	NOTE: Our current files probably need a frame so we know some about them.
	Also each block needs length

	https://github.com/lz4/lz4/issues/394

	Seems like lz4_hc can have both dictionary and look in stream but lz4 can only look at last 64kb
*/

#define MAX_LITERAL_LENGTH 255

// max_message must be at least as large as our largest state since we currently only flush between states
// this probably has to be revisted once we have 64kb states
static const uint32_t max_message = 64 * 1024;
static const uint32_t ring_buffer_threshold = 256 * 1024; // Is there any point in having this more than 64kb?
static const uint32_t ring_buffer_size = ring_buffer_threshold + max_message + 8; // This does not seem to be sufficient when ring_buffer_threshold == 64kb+max_message+8
static const uint32_t compress_buffer_size = LZ4_COMPRESSBOUND(max_message);

struct StateCompression {
	uint32_t state_size;
	uint8_t *current_state;

	LZ4_stream_t *lz4_stream;
	uint8_t *compress_buffer;

	uint8_t* input_ring_buffer;
	uint32_t input_ring_buffer_offset = 0;
	uint32_t input_ring_buffer_written_offset = 0;

	int debug_num_written = 0;
};

static void flush_ring_buffer(StateCompression &sc, FILE* output, bool flush_all = false) {

	// TODO: This ring-buffer is a bit bonkers, understand more what LZ4 actually needs to do its best in terms of buffer size
	//       Double buffer with 64kb-buffers might be as good and give easier code

	bool do_flush = sc.input_ring_buffer_offset >= ring_buffer_threshold;

	while (sc.input_ring_buffer_written_offset < sc.input_ring_buffer_offset) {
		uint32_t unwritten = sc.input_ring_buffer_offset - sc.input_ring_buffer_written_offset;
		if (unwritten == 0)
			break;

		bool wraparound = sc.input_ring_buffer_offset >= ring_buffer_threshold;

		if (!flush_all) {
			if (unwritten < max_message) {
				if (wraparound) {
					//printf("  Copy back %d bytes\n", unwritten);
					// This is to relieve the outer loop from wrapping when coping into the ring buffer
					memcpy(&sc.input_ring_buffer[0], &sc.input_ring_buffer[sc.input_ring_buffer_written_offset], unwritten);
					sc.input_ring_buffer_offset = unwritten;
					sc.input_ring_buffer_written_offset = 0;
					return;
				} else {
					// Postpone this flush
					return;
				}
			}
		}

		// If this is the last message before ring buffer wraps around, it at most max_message long and will be consumed here
		uint32_t w = std::min(unwritten, max_message);
		assert(w == max_message || !wraparound);

		const int acceleration = 0;

		uint32_t compressed_size = LZ4_compress_fast_continue(sc.lz4_stream, (const char*)&sc.input_ring_buffer[sc.input_ring_buffer_written_offset], (char*)sc.compress_buffer, w, compress_buffer_size, acceleration);
		assert(compressed_size != 0);

		//printf("  Writing block of size %d (org %d) (final offset %d)\n", compressed_size, w, sc.debug_num_written);
		fwrite(&compressed_size, 1, sizeof(compressed_size), output);
		fwrite(sc.compress_buffer, 1, compressed_size, output);

		// TODO: I think we need to reset writer pointer here or we can write too much outside... Or can we? We should be allowed
		// One more write and then it should copy back.. but anyway fix here

		sc.input_ring_buffer_written_offset += w;
		sc.debug_num_written += w;
	}
}

StateCompression* state_compression_create(uint32_t state_size) {
	StateCompression *sc = new StateCompression;
	sc->state_size = state_size;
	sc->current_state = new uint8_t[state_size];

	sc->lz4_stream = LZ4_createStream();
	
	sc->input_ring_buffer = new uint8_t[ring_buffer_size];
	sc->compress_buffer = new uint8_t[compress_buffer_size];

	memset(sc->current_state, 0, state_size);

	return sc;
}

void state_compression_destroy(StateCompression *sc, FILE *output) {
	flush_ring_buffer(*sc, output, true);
	LZ4_freeStream(sc->lz4_stream);
	delete[] sc->compress_buffer;
	delete[] sc->input_ring_buffer;
	delete [] sc->current_state;
	delete sc;
}

void state_compression_add_1(StateCompression *sc, const uint8_t * const updated_state, FILE * output, int compression_method) {
	// TODO: Write directly into ring buffer
	/*
	int literal_start = -1;
	int literal_length = 0;
	bool has_literal = false;

	uint32_t previous_literal = 0;

	const uint32_t state_size = sc->state_size;

	uint8_t* current_state = sc->current_state;

	uint8_t* rle_scratch = sc->rle_scratch_space;
	uint32_t rle_scratch_used = 0;
	
	uint8_t num_literals = 0;

	rle_scratch[rle_scratch_used] = 0;
	rle_scratch_used += sizeof(num_literals);

	for (uint32_t i=0; i<state_size; ++i) {
		bool save_current_literal = false;
		bool same = updated_state[i] == current_state[i];
		if (same && !has_literal)
			continue;

		if (same) {
			save_current_literal = true;
		} else if (!has_literal) {
			literal_start = i;
			literal_length = 1;
			has_literal = true;
		} else {
			literal_length++;
			if (literal_length == MAX_LITERAL_LENGTH)
				save_current_literal = true;
		}

		bool at_end = i + 1 == state_size;

		if (save_current_literal || (at_end && has_literal)) {
			uint32_t encoded = literal_start|(literal_length<<24);
			memcpy(&rle_scratch[rle_scratch_used], &encoded, sizeof(encoded));
			rle_scratch_used += sizeof(encoded);

			// Write literal to output
			memcpy(&rle_scratch[rle_scratch_used], &updated_state[literal_start], literal_length);
			rle_scratch_used += literal_length;

			save_current_literal = false;
			has_literal = false;
			
			assert(num_literals < 0xFF);
			num_literals++;
		}
	}

	memcpy(current_state, updated_state, state_size);

	rle_scratch[0] = num_literals;

	// Now lets lz4 compress rle_scratch together with current stream

	uint32_t compressed_size = LZ4_compress_fast_continue(sc->lz4_stream, (const char*)rle_scratch, (char*)sc->lz4_scratch_space, rle_scratch_used, sc->lz4_scatch_space_size, 0);
	fwrite(sc->lz4_scratch_space, 1, compressed_size, output);
	*/
}

// Simply compresses each block. Previous block should be in dictionary so should be a match
// This is probably tougher on decompress since the entire state needs to be desconstructed (instead of just RLE info)
// Also at some cutoff the state will be too large to be matched and then the compressed size will go way up
void state_compression_add(StateCompression *sc, const uint8_t * const updated_state, FILE * output, const uint32_t compression_method) {

	const uint32_t state_size = sc->state_size;
	uint32_t offset = sc->input_ring_buffer_offset;

	if (compression_method == 0) {
		// Uncompressed
		fwrite(updated_state, 1, state_size, output);
		return;
	}

	for(uint32_t i=0; i<state_size; ++i) {
		if (compression_method == 1) {
			sc->input_ring_buffer[offset++] = updated_state[i];
		} else if (compression_method == 2) {
			sc->input_ring_buffer[offset++] = sc->current_state[i] ^ updated_state[i];
		} else if (compression_method == 3) {
			sc->input_ring_buffer[offset++] = updated_state[i] - sc->current_state[i]; // Difference, not xor! Could be 16-bit aware for registers I guess but simple to do it on byte level
		} else {
			assert(false);
		}
	}

	// Also update internal state for compression methods that uses diff
	memcpy(sc->current_state, updated_state, sc->state_size);
	
	sc->input_ring_buffer_offset = offset; // Do not wrap here, flush_ring_buffer will do it if needed

	// Now "flush" ring buffer to output in max_message chunks
	flush_ring_buffer(*sc, output);
}

#include <vector>
void state_compression_validate(FILE *compressed, const uint32_t state_size, const uint8_t * const uncompressed, const uint32_t uncompressed_size, const uint32_t compression_method) {
	assert(compression_method!=0);

	LZ4_streamDecode_t* stream = LZ4_createStreamDecode();
	LZ4_setStreamDecode(stream, nullptr, 0);

	std::vector<uint8_t> src;
	src.resize(LZ4_COMPRESSBOUND(max_message));

	std::vector<uint8_t> state(state_size, 0);

	std::vector<uint8_t> ring_buffer;
	ring_buffer.resize(ring_buffer_size);
	uint32_t ring_buffer_offset = 0;

	uint32_t num_read = 0;

	while (true) {
		uint32_t src_size;
		uint32_t t = fread(&src_size, sizeof(uint32_t), 1, compressed);
		if (t==0) // eof of file
			break;
		assert(src_size <= src.size());
		fread(&src[0], 1, src_size, compressed);
		int decompressed_size = LZ4_decompress_safe_continue(stream, (const char*)&src[0], (char*)&ring_buffer[ring_buffer_offset], src_size, max_message);
		//printf("  Decoded block of compressed size %d, original %d (num_read=%d, uncompressed_size=%d)\n", src_size, decompressed_size, num_read, uncompressed_size);
		assert(decompressed_size <= max_message);

		// Validate decompressed data with original (assume comp
		if (compression_method == 1) {
			for (uint32_t i=0; i<decompressed_size; ++i) {
				uint32_t state_offset = (num_read + i) % state_size;
				state[state_offset] = ring_buffer[ring_buffer_offset + i];
				if (uncompressed[num_read+i] != state[state_offset]) {
					printf("Error att offset %d\n", num_read + i);
					assert(false);
				}
			}
		} else if (compression_method == 2) {
			for (uint32_t i = 0; i<decompressed_size; ++i) {
				uint32_t state_offset = (num_read + i) % state_size;
				state[state_offset] ^= ring_buffer[ring_buffer_offset + i];
				if (uncompressed[num_read + i] != state[state_offset]) {
					printf("Error att offset %d\n", num_read + i);
					assert(false);
				}
			}
		} else if (compression_method == 3) {
			for (uint32_t i = 0; i<decompressed_size; ++i) {
				uint32_t state_offset = (num_read + i) % state_size;
				state[state_offset] += ring_buffer[ring_buffer_offset + i];
				if (uncompressed[num_read + i] != state[state_offset]) {
					printf("Error att offset %d\n", num_read + i);
					assert(false);
				}
			}
		}

		ring_buffer_offset += decompressed_size;
		num_read += decompressed_size;
		if (ring_buffer_offset >= ring_buffer_threshold)
			ring_buffer_offset = 0; // We are OK to go back to the start again
	}
	
	LZ4_freeStreamDecode(stream);
}
