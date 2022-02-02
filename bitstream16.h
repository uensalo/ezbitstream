#ifndef EZBITSTREAM_BITSTREAM16_H
#define EZBITSTREAM_BITSTREAM16_H
#include "ezbitstream.h"
#include "tables.h"
namespace ezb {
    /**
     * Defines a bitstream with word size of 2 bytes
     *
     * The interface and implementation support a subset of bitvector operations such as getting, setting, or clearing
     * a bit at a particular index (but not popcount, rank, select, etc.)
     */
    class Bitstream16 {
    public:
        /**
         * Constructs a 0-based indexed bitstream of initial maximum capacity 64
         * By default, the bitstream is resizable, but can be made to be a custom constant-sized buffer
         */
        Bitstream16(UINT64 no_bits = 64);
        ~Bitstream16();
        Bitstream16(const Bitstream16 &other);
        Bitstream16 operator=(const Bitstream16 &other);

        // bit level operations
        /**
         * Sets the bit at index idx to 1
         * @param idx Index of the bit to be set
         */
        void set_bit(UINT64 idx);

        /**
         * Clears the bit at index idx to 0
         * @param idx Index of the bit to be cleared
         */
        void clear_bit(UINT64 idx);

        /**
         * Returns the bit at index idx
         * @param idx Index of the bit to be returned
         * @return True if bit is set, false otherwise
         */
        bool get_bit(UINT64 idx);

        // word level operations
        /**
         * Reads no_bits_to_read bits from the stream starting from the index denoted by start and packs the result in 2
         * bytes. The function does not advance the pointer of the stream, hence can be used for random access to the
         * bitstream.
         * @param start Index from which the read starts
         * @param no_bits_to_read Number of bits to be packed into 2 bytes, can not be more than 8. 8 by default
         * @return The bit sequence in the interval [start, start + no_bits_to_read) packed into 2 bytes padded with 0s
         */
        UINT16 read_word(UINT64 start, UINT8 no_bits_to_read=16);

        /**
         * Reads no_bits_to_read bits from the stream starting from the index denoted by the pointer of the stream and
         * packs the result in 2 bytes. The function advances the pointer of the stream by no_bits_to_read bits.
         * @param start Index from which the read starts
         * @param no_bits_to_read Number of bits to be packed into 2 bytes, can not be more than 8
         * @return The bit sequence in the interval [start, start + no_bits_to_read) packed into 2 bytes padded with 0s
         */
        UINT16 read_word(UINT8 no_bits_to_read=16);

        /**
         * Writes no_bits_to_write bits to the stream starting from the index denoted by start from the bits in "data"
         * The function does not advance the pointer of the stream, hence can be used for random writes to the stream.
         * @param start Index from which the write starts
         * @param data Data to be written to the bitstream
         * @param no_bits_to_write Number of bits to be written from data to the bitstream, starting from the lower bits
         *                         Values >8 in this field
         */
        void write_word(UINT64 start, UINT16 data, UINT8 no_bits_to_write = 8);

        /**
         * Writes no_bits_to_write bits to the stream starting from the index denoted by start from the bits in "data"
         * The function advances the pointer of the stream by no_bits_to_write.
         * @param start Index from which the write starts
         * @param data Data to be written to the bitstream
         * @param no_bits_to_write Number of bits to be written from data to the bitstream, starting from the lower bits
         */
        void write_word(UINT16 data, UINT8 no_bits_to_write=16);

        // buffer level operations
        /**
         * Writes no_bits_to_write bits from the buffer pointed to with data to the bitstream starting from the index
         * start. Does not advance the pointer of the stream, hence it can be used for random writes
         * @param start Index from which the write starts
         * @param data Pointer to the buffer whose contents are to be copied to the bitstream
         * @param data_size Size of the buffer indicated with data
         * @param no_bits_to_write Number of bits to be written to the bitstream
         */
        void write_buffer(UINT64 start, UINT16 *data, UINT64 data_size, UINT64 no_bits_to_write);

        /**
         * Writes no_bits_to_write bits from the buffer pointed to with data to the bitstream starting from the pointer
         * of the bitstream. Advances the pointer of the bitstream by no_bits_to_write bits
         * @param data Pointer to the buffer whose contents are to be copied to the bitstream
         * @param data_size Size of the buffer indicated with data
         * @param no_bits_to_write Number of bits to be written to the bitstream
         */
        void write_buffer(UINT16 *data, UINT64 data_size, UINT64 no_bits_to_write);

        // stream level operations
        /**
         * Writes no_bits_to_write bits from the buffer pointed to with data to the bitstream starting from the pointer
         * of the bitstream. Does not advance the pointer of either of the streams.
         * @param start_destination Starting index of the destination stream
         * @param start_source Starting index of the source stream
         * @param no_bits_to_write Number of bits to copy from the source stream
         * @param source Reference to the source bitstream
         */
        void write_stream(UINT64 start_destination, UINT64 start_source, UINT64 no_bits_to_write, Bitstream16 &source);

        /**
         * Writes no_bits_to_write bits from the stream pointed to by source to the bitstream starting from the pointer
         * at the bitstream, and start_source of the source bitstream. Advances the pointer of the destination
         * bitstream, but not that of the source bitstream
         * @param start_source Starting index of the source stream
         * @param no_bits_to_write Number of bits to copy from the source stream
         * @param source Reference to the source bitstream
         */
        void write_stream(UINT64 start_source, UINT64 no_bits_to_write, Bitstream16 &source);

        /**
         * Writes no_bits_to_write bits from the stream pointed to by source to the bitstream starting from the pointer
         * of the bitstream. Advances the pointers of both bitstreams
         * @param no_bits_to_write Number of bits to copy from the source stream
         * @param source Reference to the source bitstream
         */
        void write_stream(UINT64 no_bits_to_write, Bitstream16 &source);

        /**
         * Returns a reference to the buffer of the bitstream and the size of the buffer in words. Allocates a new
         * buffer for the bitstream object. Return values are through the parameter list
         * @param buffer Reference to the buffer of the bitstream
         * @param size Size of the buffer of the bitstream
         */
        void flush(UINT16 *&buffer, UINT64 &size, UINT64 new_capacity=64);

        //pointer operations
        /**
         * Increments the pointer of the stream denoted by increment with maximum value clamped to bit capacity
         * @param increment Increment to be applied in the number of bits
         */
        void increment_pointer(UINT64 increment);

        /**
         * Decrements the pointer of the stream denoted by decrement with minimum value clamped to 0
         * @param decrement Decrement to be applied in the number of bits
         */
        void decrement_pointer(UINT64 decrement);

        /**
         * Sets the position of the pointer denoted by the index, with max value clamped to bit capacity
         * @param index Index of the stream to be set
         */
        void set_pointer(UINT64 index);

        /**
         * Returns the index of the pointer into the bitstream
         * @return Pointer index
         */
        UINT64 pointer();

        /**
         * Returns the capacity of the buffer of the bitstream
         */
        UINT64 capacity();

    private:
        /**
         * Doubles the capacity of the buffer in case no_bits_to_write > (m_capacity)
         */
        void double_capacity();

        UINT64 m_pointer;
        UINT16  *m_two_bytes;
        UINT64 m_capacity;
    };
}
#endif //EZBITSTREAM_BITSTREAM16_H
