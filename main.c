#include "lfsr.h"


int main(int argc, char** argv) {
    size_t collisions = 0;
    const uint16_t initial_state = LFSR_BIT_INIT;
    const uint8_t INITIAL_SEED_SIZE = (uint8_t)(sizeof(initial_state) * 8);
    tBit bit_sequence[MAX_SEQ_SIZE] = {0};
    size_t seq_size = INITIAL_SEED_SIZE;

    /* Initialize bit_sequence from initial_state */
    for (size_t i = 0; i < seq_size; ++i) {
        bit_sequence[i] = (initial_state >> i) & 1;
    }
    uint8_t tap_A = 0, tap_B = seq_size - 1;
    uint64_t cycle_threshold = (1ULL << seq_size);

    printf("Using initial state: %u\n", (unsigned)initial_state);

    for (uint64_t cycle = 0; cycle <= cycle_threshold; ++cycle) {
        printf("%llu/%llu) %d %d = ", (unsigned long long)cycle, (unsigned long long)cycle_threshold, tap_A, tap_B);
        print_sequence(bit_sequence, seq_size);
        printf("\n");

        /* Collision detection */
        if (collisionDetection(seq_size, bit_sequence, seq_size)) {
            collisions++;
            printf("Collision Detected: %llu/%llu) %d %d = ", (unsigned long long)cycle, (unsigned long long)cycle_threshold, tap_A, tap_B);
            print_sequence(bit_sequence, seq_size);
            printf("\n");
        }

        registerClockSequence(seq_size, bit_sequence, seq_size);

        tBit feedback = bit_sequence[tap_A] ^ bit_sequence[tap_B];

        /* Shift right and insert feedback at the left */
        for (size_t i = seq_size - 1; i > 0; --i) {
            bit_sequence[i] = bit_sequence[i - 1];
        }
        bit_sequence[0] = feedback;
        registerFeedback(feedback, seq_size, tap_A, tap_B);

        /* Change tap positions at end of threshold */
        if (cycle >= cycle_threshold - 1) {
            /* Re-initialize (optionally, could randomize or advance state) */
            printf("Old State: ");
            print_sequence(bit_sequence, seq_size);
            printf("\n");
            /* Simple tap advancing logic */
            if (tap_A < seq_size - 1) {
                tap_A++;
                if (tap_A == tap_B) tap_B = (tap_B > 0) ? tap_B - 1 : 0;
            } else if (tap_B > 0) {
                tap_B--;
            } else {
                break;
            }
            cycle = 0;
        }
    }

    finalizeSequenceStateStream();

    /* Count filled StateMap entries */
    size_t state_count = 0;
    for (size_t i = 0; i < MAX_SEQ_SIZE; ++i) {
        if (StateMap[i].block_stream_capacity > 0) state_count++;
    }
    printf("Feedback Map Size = %zu\n", state_count);
    printf("Calculated Stream Output: %llu\n", (unsigned long long)output_stream);
    printf("Total Collisions: %zu\n", collisions);

    /* Cleanup */
    for (size_t i = 0; i < MAX_SEQ_SIZE; ++i) {
        if (StateMap[i].block_stream_capacity > 0) SequenceStates_free(&StateMap[i]);
    }

    return 0;
}
