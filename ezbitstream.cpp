#include "ezbitstream.h"
using namespace ezb;
Bitstream8::Bitstream8() {
    m_no_bits = 0;
    m_pointer = 0;
    m_bytes = new UINT8[8];
    m_capacity = 8;
}

Bitstream8::~Bitstream8() {
    delete m_bytes;
}

void Bitstream8::set_bit(UINT64 idx) {
    m_bytes[(idx >> 3)] |= 0b1 << (idx & 0b111ull);
}

void Bitstream8::clear_bit(UINT64 idx) {
    m_bytes[(idx >> 3)] &= ~(0b1 << ((idx & 0b111ull)));
}

bool Bitstream8::get_bit(UINT64 idx) {
    return (m_bytes[(idx >> 3)] & (0b1 << (idx & 0b111ull))) > 0;
}

UINT8 Bitstream8::read_word(UINT64 start, UINT8 no_bits_to_read) {
    UINT64 byte_idx = start >> 3;
    UINT64 bit_start_offset = start & 0b111ull;
    UINT64 bit_end_offset = (no_bits_to_read - start + 1) & 0b111ull;
    if (bit_start_offset == 0) { // byte aligned read of no_bits_to_read many bits
        return m_bytes[byte_idx] & MASK_SHIFT_8_RIGHT[8-no_bits_to_read];
    }
    if (bit_end_offset >= bit_start_offset) { // end and start are in the same byte, extract and return middle bits
        return (m_bytes[byte_idx] & (MASK_SHIFT_8_LEFT[bit_start_offset] & MASK_SHIFT_8_RIGHT[7 - bit_end_offset])) >> bit_start_offset;
    }
    // else, the read is split on two bytes: read the relevant section from both and combine
    return  ((m_bytes[byte_idx] & MASK_SHIFT_8_LEFT[bit_start_offset]) >> (bit_start_offset)) |
            ((m_bytes[byte_idx + 1] & MASK_SHIFT_8_RIGHT[7 - bit_end_offset]) << (8-bit_end_offset));
}

UINT8 Bitstream8::read_word(UINT8 no_bits_to_read) {
    UINT64 byte_idx = m_pointer >> 3;
    UINT64 bit_start_offset = m_pointer & 0b111ull;
    UINT64 bit_end_offset = (no_bits_to_read - m_pointer + 1) & 0b111ull;
    if (bit_start_offset == 0) { // byte aligned read of no_bits_to_read many bits
        return m_bytes[byte_idx] & MASK_SHIFT_8_RIGHT[8-no_bits_to_read];
    }
    if (bit_end_offset >= bit_start_offset) { // end and start are in the same byte, extract and return middle bits
        return (m_bytes[byte_idx] & (MASK_SHIFT_8_LEFT[bit_start_offset] & MASK_SHIFT_8_RIGHT[7 - bit_end_offset])) >> bit_start_offset;
    }
    // else, the read is split on two bytes: read the relevant section from both and combine
    return  ((m_bytes[byte_idx] & MASK_SHIFT_8_LEFT[bit_start_offset]) >> (bit_start_offset)) |
            ((m_bytes[byte_idx + 1] & MASK_SHIFT_8_RIGHT[7 - bit_end_offset]) << (8-bit_end_offset));
}

void Bitstream8::write_word(UINT64 start, UINT8 data, UINT8 no_bits_to_write) {
    UINT64 byte_idx = start >> 3;
    UINT64 bit_start_offset = start & 0b111ull;
    UINT64 bit_end_offset = (start + no_bits_to_write - 1) & 0b111ull;

    if (bit_start_offset == 0) { // word aligned write, write no_bits_to_write many bits from data
        m_bytes[byte_idx] &= MASK_SHIFT_8_LEFT[no_bits_to_write];
        m_bytes[byte_idx] |= data & MASK_SHIFT_8_RIGHT[8 - no_bits_to_write];
        return;
    }
    if(bit_end_offset >= bit_start_offset) { // write into a single word, clear the middle bits
        UINT8 mask = MASK_SHIFT_8_LEFT[bit_start_offset] & MASK_SHIFT_8_RIGHT[7-bit_end_offset];
        m_bytes[byte_idx] &= ~mask;
        m_bytes[byte_idx] |= mask & (data << (bit_start_offset));
        return;
    }
    // write to two adjacent words
    m_bytes[byte_idx] &= ~MASK_SHIFT_8_LEFT[bit_start_offset];
    m_bytes[byte_idx++] |= (data << bit_start_offset);
    m_bytes[byte_idx] &= MASK_SHIFT_8_LEFT[++bit_end_offset];
    m_bytes[byte_idx] |= (data >> (8 - bit_start_offset)) & ~MASK_SHIFT_8_LEFT[bit_end_offset];
}

void Bitstream8::write_word(UINT8 data, UINT8 no_bits_to_write) {
    UINT64 byte_idx = m_pointer >> 3;
    UINT64 bit_start_offset = m_pointer & 0b111ull;
    UINT64 bit_end_offset = (m_pointer + no_bits_to_write - 1) & 0b111ull;

    if (bit_start_offset == 0) { // word aligned write, write no_bits_to_write many bits from data
        m_bytes[byte_idx] &= MASK_SHIFT_8_LEFT[no_bits_to_write];
        m_bytes[byte_idx] |= data & MASK_SHIFT_8_RIGHT[8 - no_bits_to_write];
        m_pointer += no_bits_to_write;
        return;
    }
    if(bit_end_offset >= bit_start_offset) { // write into a single word, clear the middle bits
        UINT8 mask = MASK_SHIFT_8_LEFT[bit_start_offset] & MASK_SHIFT_8_RIGHT[7-bit_end_offset];
        m_bytes[byte_idx] &= ~mask;
        m_bytes[byte_idx] |= mask & (data << (bit_start_offset));
        m_pointer += no_bits_to_write;
        return;
    }
    // write to two adjacent words
    m_bytes[byte_idx] &= ~MASK_SHIFT_8_LEFT[bit_start_offset];
    m_bytes[byte_idx++] |= (data << bit_start_offset);
    m_bytes[byte_idx] &= MASK_SHIFT_8_LEFT[++bit_end_offset];
    m_bytes[byte_idx] |= (data >> (8 - bit_start_offset)) & ~MASK_SHIFT_8_LEFT[bit_end_offset];
    m_pointer += no_bits_to_write;
}

void Bitstream8::write_buffer(UINT64 start, UINT8* data, UINT64 data_size, UINT64 no_bits_to_write) {
    UINT64 bit_offset = start & 0b111ull;
    if (bit_offset) { // unaligned write to the bitstream
        // write the first no_bits_to_write / 8 words to the bitstream
        UINT64 cur_byte = start >> 3;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write << 3); i++) {
            m_bytes[cur_byte] &= ~MASK_SHIFT_8_LEFT[bit_offset];
            m_bytes[cur_byte++] |= (data[i] << bit_offset);
            m_bytes[cur_byte] &= MASK_SHIFT_8_LEFT[bit_offset - 1];
            m_bytes[cur_byte] |= (data[i] >> (8 - bit_offset)) & ~MASK_SHIFT_8_LEFT[bit_offset];
        }
        // write remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset + bits_left - 1) & 0b111ull;
            if (bit_offset <= end_offset) {
                UINT8 mask = MASK_SHIFT_8_LEFT[bit_offset] & MASK_SHIFT_8_RIGHT[7-end_offset];
                m_bytes[cur_byte] &= ~mask;
                m_bytes[cur_byte] |= mask & (data[i] << (bit_offset));
                return;
            } else {
                // unaligned write split on the first and second byte
                m_bytes[cur_byte] &= ~MASK_SHIFT_8_LEFT[bit_offset];
                m_bytes[cur_byte++] |= (data[i] << bit_offset);
                m_bytes[cur_byte] &= MASK_SHIFT_8_LEFT[++end_offset];
                m_bytes[cur_byte] |= (data[i] >> (8 - bit_offset)) & ~MASK_SHIFT_8_LEFT[end_offset];
            }
        }
    } else { // aligned write on the bitstream
        UINT64 cur_byte = start >> 3;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write >> 3); i++) {
            m_bytes[cur_byte++] = data[i];
        }
        // write the remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            m_bytes[cur_byte] &= MASK_SHIFT_8_LEFT[bits_left];
            m_bytes[cur_byte] |= data[i] & MASK_SHIFT_8_RIGHT[8 - bits_left];
        }
    }
}

void Bitstream8::write_buffer(UINT8* data, UINT64 data_size, UINT64 no_bits_to_write) {
    UINT64 bit_offset = m_pointer & 0b111ull;
    if (bit_offset) { // unaligned write to the bitstream
        // write the first no_bits_to_write / 8 words to the bitstream
        UINT64 cur_byte = m_pointer >> 3;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write << 3); i++) {
            m_bytes[cur_byte] &= ~MASK_SHIFT_8_LEFT[bit_offset];
            m_bytes[cur_byte++] |= (data[i] << bit_offset);
            m_bytes[cur_byte] &= MASK_SHIFT_8_LEFT[bit_offset - 1];
            m_bytes[cur_byte] |= (data[i] >> (8 - bit_offset)) & ~MASK_SHIFT_8_LEFT[bit_offset];
        }
        // write remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset + bits_left - 1) & 0b111ull;
            if (bit_offset <= end_offset) {
                UINT8 mask = MASK_SHIFT_8_LEFT[bit_offset] & MASK_SHIFT_8_RIGHT[7-end_offset];
                m_bytes[cur_byte] &= ~mask;
                m_bytes[cur_byte] |= mask & (data[i] << (bit_offset));
                return;
            } else {
                // unaligned write split on the first and second byte
                m_bytes[cur_byte] &= ~MASK_SHIFT_8_LEFT[bit_offset];
                m_bytes[cur_byte++] |= (data[i] << bit_offset);
                m_bytes[cur_byte] &= MASK_SHIFT_8_LEFT[++end_offset];
                m_bytes[cur_byte] |= (data[i] >> (8 - bit_offset)) & ~MASK_SHIFT_8_LEFT[end_offset];
            }
        }
    } else { // aligned write on the bitstream
        UINT64 cur_byte = m_pointer >> 3;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write >> 3); i++) {
            m_bytes[cur_byte++] = data[i];
        }
        // write the remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            m_bytes[cur_byte] &= MASK_SHIFT_8_LEFT[bits_left];
            m_bytes[cur_byte] |= data[i] & MASK_SHIFT_8_RIGHT[8 - bits_left];
        }
    }
    m_pointer += no_bits_to_write;
}

void write_stream(UINT64 start_destination, UINT64 start_source, UINT64 no_bits_to_write, const Bitstream8 &source) {}

void write_stream(UINT64 start_source, UINT64 no_bits_to_write, const Bitstream8 &source) {}

void write_stream(UINT64 no_bits_to_write, const Bitstream8 &source) {}

void flush(UINT8* &buffer, UINT64 &size) {}