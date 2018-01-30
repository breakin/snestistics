#include "state_compression.h"
#include <vector>
#include <stdint.h>


void state_compression_destroy(StateCompression *sc, FILE *output);

int main(void) {

	static const int STATE_SIZE = 1024*64;

	for (int method = 1; method<=3; method++) {
		printf("Testing method %d\n", method);
		StateCompression *sc = state_compression_create(STATE_SIZE);
		FILE *output = fopen("test.temp", "wb");

		srand(12345);

		std::vector<uint8_t> reference;
		static const int NUM_BLOCKS = 2000;
		reference.reserve(NUM_BLOCKS*STATE_SIZE);
		
		std::vector<uint8_t> state(STATE_SIZE, 0);

		for (int blocks = 0; blocks<NUM_BLOCKS; blocks++) {
			int changes = 1 + (rand() % 20);
			for (int j=0; j<changes; j++) {
				int pos = rand() % STATE_SIZE;
				state[pos] = rand();
				//printf("  Change %d of %d, pos=%d\n", j+1, changes, pos);
			}
			for (int j=0;j<STATE_SIZE; j++) {
				reference.push_back(state[j]);
			}
			state_compression_add(sc, &state[0], output, method);
		}

		state_compression_destroy(sc, output);

		int size = ftell(output);
		fclose(output);

		printf("  Size %d (%.2f%%)\n", size, size*100.0f/reference.size());

		// Validate
		output = fopen("test.temp", "rb");
		state_compression_validate(output, STATE_SIZE, &reference[0], (uint32_t)reference.size(), method);
		fclose(output);
	}

	return 0;
}
