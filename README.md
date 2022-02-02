# ezbitstream
A simple C++ library for creating and managing bitstreams in memory.

## API & Implementation
ezbitstream implements bitstreams with word sizes 8, 16, 32, and 64 bits. The operations supported by the data structure are as follows:

- Read, set, clear single bits
- Read and write words with random access, both word-aligned and non-aligned
- Write buffers to the bitstream with random access, both word-aligned and non-aligned
- Write to/from other bitstreams with random access, both word-aligned and non-aligned
- Flush buffer back to the user

The bitstream itself is implemented as a 0-based indexed dynamic buffer of 8, 16, 32, 64 bit words depending on the type.

The implementation is meant to be as self contained as possible, with the only external dependency being stdint.h.

Implementations of individual bitstreams of word size X are given under bitstreamX.h
