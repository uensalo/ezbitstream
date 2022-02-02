#ifndef _EZBITSTREAM_H
#define _EZBITSTREAM_H

#include <stdint.h>

/**
 * Type definitions denoting the size of concurrent access to the buffer
 */
namespace ezb {
    typedef uint64_t UINT64;
    typedef uint32_t UINT32;
    typedef uint16_t UINT16;
    typedef uint8_t UINT8;
}

/**
 * Class definitions for bitstreams of various size of concurrent access
 */
namespace ezb {
    class Bitstream8;
    class Bitstream16;
    class Bitstream32;
    class Bitstream64;
}

#endif
