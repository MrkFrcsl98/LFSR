#ifndef LFSR_H
#define LFSR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// Attribute macros for hints to the compiler, mostly for GCC/clang
#if defined(__GNUC__) || defined(__clang__)
#define __attr_nodiscard __attribute__((warn_unused_result)) // don’t ignore the result!
#define __attr_malloc __attribute__((malloc)) // kinda says “this returns fresh memory”
#define __attr_hot __attribute__((hot)) // function probably called a lot
#define __attr_cold __attribute__((cold)) // probably NOT called very much
#define likely(x)   __builtin_expect(!!(x), 1) // branch prediction hint
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define __attr_nodiscard
#define __attr_malloc
#define __attr_hot
#define __attr_cold
#define likely(x)   (x)
#define unlikely(x) (x)
#endif

#ifdef __cplusplus
#define __restrict__ __restrict
#define __noexcept noexcept
#define __const_noexcept const noexcept
#else
#define __restrict__ restrict
#define __noexcept
#define __const_noexcept
#endif

#define LFSR_BIT_INIT 0x0af0U // Just a default starting value
#define MAX_SEQ_SIZE 64       // Max size we support for LFSR

typedef uint8_t tBit; // “bit” is just a fancy way to say uint8_t

/**
 * This struct keeps track of everything for a single sequence size.
 * It’s got the bitstream, some arrays for tracking stuff, and two extra numbers.
 */
typedef struct {
    tBit* block_stream;             
    size_t block_stream_len;        
    size_t block_stream_capacity;   

    tBit** clock_sequence;          
    size_t* clock_sequence_lens;    
    size_t clock_sequence_count;    
    size_t clock_sequence_capacity; 

    uint8_t tA, tB; // not sure what these are for, but they get filled in
} SequenceStates;

// Here’s a fixed-size array for all our states, and an output thing
static SequenceStates StateMap[MAX_SEQ_SIZE];
static uint64_t output_stream = 0;

/**
 * Sets up a SequenceStates struct so it’s ready to use.
 * Everything gets set to zero or NULL.
 */
__attr_hot static inline void SequenceStates_init(
    SequenceStates* __restrict__ ss, uint8_t tA, uint8_t tB
) __noexcept {
    ss->block_stream = NULL;
    ss->block_stream_len = 0;
    ss->block_stream_capacity = 0;
    ss->clock_sequence = NULL;
    ss->clock_sequence_lens = NULL;
    ss->clock_sequence_count = 0;
    ss->clock_sequence_capacity = 0;
    ss->tA = tA;
    ss->tB = tB;
}

/**
 * Frees up anything we malloc’d in the SequenceStates.
 * Does nothing if it’s already empty/null.
 */
__attr_cold static inline void SequenceStates_free(
    SequenceStates* __restrict__ ss
) __noexcept {
    free(ss->block_stream);
    // Only bother if clock_sequence is actually set
    if (likely(ss->clock_sequence)) {
        for (size_t i = 0; i < ss->clock_sequence_count; ++i) {
            free(ss->clock_sequence[i]);
        }
        free(ss->clock_sequence);
        free(ss->clock_sequence_lens);
    }
}

/**
 * Makes sure there’s enough room in the block_stream array.
 * If not, makes it bigger (doubles it, or starts at 16).
 */
__attr_hot static inline void ensure_block_stream_capacity(
    SequenceStates* __restrict__ ss
) __noexcept {
    if (unlikely(ss->block_stream_len >= ss->block_stream_capacity)) {
        size_t new_cap = (ss->block_stream_capacity == 0) ? 16 : ss->block_stream_capacity * 2;
        ss->block_stream = (tBit*)realloc(ss->block_stream, new_cap * sizeof(tBit));
        ss->block_stream_capacity = new_cap;
    }
}

/**
 * Makes sure there’s enough room to add another clock sequence.
 * If not, resizes the arrays (doubles, or starts at 8).
 */
__attr_hot static inline void ensure_clock_sequence_capacity(
    SequenceStates* __restrict__ ss
) __noexcept {
    if (unlikely(ss->clock_sequence_count >= ss->clock_sequence_capacity)) {
        size_t new_cap = (ss->clock_sequence_capacity == 0) ? 8 : ss->clock_sequence_capacity * 2;
        ss->clock_sequence = (tBit**)realloc(ss->clock_sequence, new_cap * sizeof(tBit*));
        ss->clock_sequence_lens = (size_t*)realloc(ss->clock_sequence_lens, new_cap * sizeof(size_t));
        ss->clock_sequence_capacity = new_cap;
    }
}

/**
 * Adds a feedback bit to the state for a particular size.
 * If we haven’t set it up yet, it calls the init function.
 */
__attr_hot static inline void registerFeedback(
    tBit feedback, size_t seq_size, uint8_t tA, uint8_t tB
) __noexcept {
    SequenceStates* ss = &StateMap[seq_size];
    if (unlikely(ss->block_stream_capacity == 0)) {
        SequenceStates_init(ss, tA, tB);
    }
    ensure_block_stream_capacity(ss);
    ss->block_stream[ss->block_stream_len++] = feedback;
}

/**
 * Adds a whole clock sequence to the state.
 * Makes a copy of your sequence, so you don’t have to keep it around.
 */
__attr_hot static inline void registerClockSequence(
    size_t seq_size, const tBit* __restrict__ sequence, size_t sequence_len
) __noexcept {
    SequenceStates* ss = &StateMap[seq_size];
    if (unlikely(ss->clock_sequence_capacity == 0)) {
        SequenceStates_init(ss, 0, 0); // zeroes for tA and tB (not used here)
    }
    ensure_clock_sequence_capacity(ss);
    // Copy the sequence so the caller doesn’t have to care about freeing it
    tBit* seq_copy = (tBit*)malloc(sequence_len * sizeof(tBit));
    memcpy(seq_copy, sequence, sequence_len * sizeof(tBit));
    ss->clock_sequence[ss->clock_sequence_count] = seq_copy;
    ss->clock_sequence_lens[ss->clock_sequence_count] = sequence_len;
    ss->clock_sequence_count++;
}

/**
 * Checks if a given sequence matches the saved block stream.
 * Returns 1 if it’s a match, 0 if not.
 * Don’t ignore the result — it’s important!
 */
__attr_nodiscard static inline int collisionDetection(
    size_t seq_size, const tBit* __restrict__ seq, size_t seq_len
) __noexcept {
    SequenceStates* ss = &StateMap[seq_size];
    for (size_t i = 0; i < ss->block_stream_len && i < seq_len; ++i) {
        if (unlikely(ss->block_stream[i] != seq[i])) return 0;
    }
    // Only return “match” if lengths are exactly equal
    return (ss->block_stream_len == seq_len);
}

/**
 * Pads all block_streams to a byte boundary (multiples of 8), then
 * adds up the contents into output_stream as bytes.
 * This is mostly for “finishing up” at the end.
 */
__attr_cold static inline void finalizeSequenceStateStream(void) __noexcept {
    for (size_t seq_size = 0; seq_size < MAX_SEQ_SIZE; ++seq_size) {
        SequenceStates* ss = &StateMap[seq_size];
        if (unlikely(ss->block_stream_capacity == 0)) continue;
        // Add zeros at the end to make length a multiple of 8
        while (ss->block_stream_len % 8 != 0) {
            ensure_block_stream_capacity(ss);
            ss->block_stream[ss->block_stream_len++] = 0;
        }
        // For every 8 bits, make a byte and add it to the output_stream
        for (size_t c = 0; c < ss->block_stream_len; c += 8) {
            uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) {
                byte |= (ss->block_stream[c + i] & 1) << (7-i);
            }
            output_stream += byte;
        }
    }
}

/**
 * Prints out a sequence of bits, one after another. No spaces, no newline.
 */
static inline void print_sequence(
    const tBit* __restrict__ seq, size_t len
) __noexcept {
    for (size_t i = 0; i < len; ++i) {
        printf("%d", seq[i] ? 1 : 0);
    }
}

#endif /* LFSR_H */
