#include "bitstream8.h"
using namespace ezb;

Bitstream8::Bitstream8(UINT64 no_bits) {
    m_pointer = 0;
    m_capacity = (no_bits >> 3) == 0 ? 1 : (no_bits >> 3);
    m_bytes = new UINT8[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_bytes[i] = 0;
    }
}

Bitstream8::~Bitstream8() {
    delete m_bytes;
}

Bitstream8::Bitstream8(const Bitstream8 &other) {
    m_pointer = other.m_pointer;
    m_capacity = other.m_capacity;
    m_bytes = new UINT8[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_bytes[i] = other.m_bytes[i];
    }
}

Bitstream8 Bitstream8::operator=(const Bitstream8 &other) {
    m_pointer = other.m_pointer;
    m_capacity = other.m_capacity;
    m_bytes = new UINT8[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_bytes[i] = other.m_bytes[i];
    }
    return *this;
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
    while(start + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
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
    while(m_pointer + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
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
    while(start + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
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
    while(m_pointer + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
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

void Bitstream8::write_stream(UINT64 start_destination, UINT64 start_source, UINT64 no_bits_to_write, Bitstream8 &source) {
    if(start_source + no_bits_to_write > (source.m_capacity << 3)) { // reading from unallocated memory
        return;
    }
    while(start_destination + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
    // try to word align the source by writing 8 - bit_offset_src bits
    UINT64 initial_bits_to_write = no_bits_to_write < 8 - (start_source & 0b111ull) ? no_bits_to_write : 8 - no_bits_to_write;
    UINT8 initial_source_data = source.read_word(start_source, initial_bits_to_write);
    write_word(start_destination, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    start_destination += initial_bits_to_write;
    start_source += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = start_destination & 0b111ull;
    //UINT64 bit_offset_src = start_source & 0b111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_byte_dst = start_destination >> 3;
        UINT64 cur_byte_src = start_source >> 3;
        for (i = 0; i < (no_bits_to_write >> 3); i++) { // just copy the words until boundary
            m_bytes[cur_byte_dst++] = source.m_bytes[cur_byte_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if(bits_left) { // write the remaining bits to src
            m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[bits_left];
            m_bytes[cur_byte_dst] |= source.m_bytes[cur_byte_src] & MASK_SHIFT_8_RIGHT[8 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_byte_dst = start_destination >> 3;
        UINT64 cur_byte_src = start_source >> 3;
        for(i = 0; i < (no_bits_to_write >> 3); i++) { // writes will always span two words
            m_bytes[cur_byte_dst] &= ~MASK_SHIFT_8_LEFT[bit_offset_dst];
            m_bytes[cur_byte_dst++] |= (source.m_bytes[cur_byte_src] << bit_offset_dst);
            m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[bit_offset_dst - 1];
            m_bytes[cur_byte_dst] |= (source.m_bytes[cur_byte_src++] >> (8 - bit_offset_dst)) & ~MASK_SHIFT_8_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b111ull;
            if (bit_offset_dst <= end_offset) {
                UINT8 mask = MASK_SHIFT_8_LEFT[bit_offset_dst] & MASK_SHIFT_8_RIGHT[7-end_offset];
                m_bytes[cur_byte_dst] &= ~mask;
                m_bytes[cur_byte_dst] |= mask & (source.m_bytes[cur_byte_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second byte
                m_bytes[cur_byte_dst] &= ~MASK_SHIFT_8_LEFT[bit_offset_dst];
                m_bytes[cur_byte_dst++] |= (source.m_bytes[cur_byte_src]  << bit_offset_dst);
                m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[++end_offset];
                m_bytes[cur_byte_dst] |= (source.m_bytes[cur_byte_src] >> (8 - bit_offset_dst)) & ~MASK_SHIFT_8_LEFT[end_offset];
            }
        }
    }
}

void Bitstream8::write_stream(UINT64 start_source, UINT64 no_bits_to_write, Bitstream8 &source) {
    if(start_source + no_bits_to_write > (source.m_capacity << 3)) { // reading from unallocated memory
        return;
    }
    while(m_pointer + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
    // try to word align the source by writing 8 - bit_offset_src bits
    UINT64 initial_bits_to_write = no_bits_to_write < 8 - (start_source & 0b111ull) ? no_bits_to_write : 8 - no_bits_to_write;
    UINT8 initial_source_data = source.read_word(start_source, initial_bits_to_write);
    write_word(m_pointer, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    m_pointer += initial_bits_to_write;
    start_source += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = m_pointer & 0b111ull;
    //UINT64 bit_offset_src = start_source & 0b111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_byte_dst = m_pointer >> 3;
        UINT64 cur_byte_src = start_source >> 3;
        for (i = 0; i < (no_bits_to_write >> 3); i++) { // just copy the words until boundary
            m_bytes[cur_byte_dst++] = source.m_bytes[cur_byte_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if(bits_left) { // write the remaining bits to src
            m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[bits_left];
            m_bytes[cur_byte_dst] |= source.m_bytes[cur_byte_src] & MASK_SHIFT_8_RIGHT[8 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_byte_dst = m_pointer >> 3;
        UINT64 cur_byte_src = start_source >> 3;
        for(i = 0; i < (no_bits_to_write >> 3); i++) { // writes will always span two words
            m_bytes[cur_byte_dst] &= ~MASK_SHIFT_8_LEFT[bit_offset_dst];
            m_bytes[cur_byte_dst++] |= (source.m_bytes[cur_byte_src] << bit_offset_dst);
            m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[bit_offset_dst - 1];
            m_bytes[cur_byte_dst] |= (source.m_bytes[cur_byte_src++] >> (8 - bit_offset_dst)) & ~MASK_SHIFT_8_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b111ull;
            if (bit_offset_dst <= end_offset) {
                UINT8 mask = MASK_SHIFT_8_LEFT[bit_offset_dst] & MASK_SHIFT_8_RIGHT[7-end_offset];
                m_bytes[cur_byte_dst] &= ~mask;
                m_bytes[cur_byte_dst] |= mask & (source.m_bytes[cur_byte_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second byte
                m_bytes[cur_byte_dst] &= ~MASK_SHIFT_8_LEFT[bit_offset_dst];
                m_bytes[cur_byte_dst++] |= (source.m_bytes[cur_byte_src]  << bit_offset_dst);
                m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[++end_offset];
                m_bytes[cur_byte_dst] |= (source.m_bytes[cur_byte_src] >> (8 - bit_offset_dst)) & ~MASK_SHIFT_8_LEFT[end_offset];
            }
        }
    }
    m_pointer += no_bits_to_write;
}

void Bitstream8::write_stream(UINT64 no_bits_to_write, Bitstream8 &source) {
    if(source.m_pointer + no_bits_to_write > (source.m_capacity << 3)) { // reading from unallocated memory
        return;
    }
    while(m_pointer + no_bits_to_write > (m_capacity << 3)) {
        double_capacity();
    }
    // try to word align the source by writing 8 - source.m_pointer bits
    UINT64 initial_bits_to_write = no_bits_to_write < 8 - (source.m_pointer & 0b111ull) ? no_bits_to_write : 8 - no_bits_to_write;
    UINT8 initial_source_data = source.read_word(source.m_pointer, initial_bits_to_write);
    write_word(m_pointer, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    m_pointer += initial_bits_to_write;
    source.m_pointer += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = m_pointer & 0b111ull;
    //UINT64 bit_offset_src = start_source & 0b111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_byte_dst = m_pointer >> 3;
        UINT64 cur_byte_src = source.m_pointer >> 3;
        for (i = 0; i < (no_bits_to_write >> 3); i++) { // just copy the words until boundary
            m_bytes[cur_byte_dst++] = source.m_bytes[cur_byte_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if(bits_left) { // write the remaining bits to src
            m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[bits_left];
            m_bytes[cur_byte_dst] |= source.m_bytes[cur_byte_src] & MASK_SHIFT_8_RIGHT[8 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_byte_dst = m_pointer >> 3;
        UINT64 cur_byte_src = source.m_pointer >> 3;
        for(i = 0; i < (no_bits_to_write >> 3); i++) { // writes will always span two words
            m_bytes[cur_byte_dst] &= ~MASK_SHIFT_8_LEFT[bit_offset_dst];
            m_bytes[cur_byte_dst++] |= (source.m_bytes[cur_byte_src] << bit_offset_dst);
            m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[bit_offset_dst - 1];
            m_bytes[cur_byte_dst] |= (source.m_bytes[cur_byte_src++] >> (8 - bit_offset_dst)) & ~MASK_SHIFT_8_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b111ull;
            if (bit_offset_dst <= end_offset) {
                UINT8 mask = MASK_SHIFT_8_LEFT[bit_offset_dst] & MASK_SHIFT_8_RIGHT[7-end_offset];
                m_bytes[cur_byte_dst] &= ~mask;
                m_bytes[cur_byte_dst] |= mask & (source.m_bytes[cur_byte_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second byte
                m_bytes[cur_byte_dst] &= ~MASK_SHIFT_8_LEFT[bit_offset_dst];
                m_bytes[cur_byte_dst++] |= (source.m_bytes[cur_byte_src]  << bit_offset_dst);
                m_bytes[cur_byte_dst] &= MASK_SHIFT_8_LEFT[++end_offset];
                m_bytes[cur_byte_dst] |= (source.m_bytes[cur_byte_src] >> (8 - bit_offset_dst)) & ~MASK_SHIFT_8_LEFT[end_offset];
            }
        }
    }
    m_pointer += no_bits_to_write;
    source.m_pointer += no_bits_to_write;
}

void Bitstream8::flush(UINT8* &buffer, UINT64 &size, UINT64 new_capacity) {
    buffer = m_bytes;
    size = m_capacity;
    m_bytes = new UINT8[new_capacity];
    m_capacity = new_capacity;
}

void Bitstream8::increment_pointer(UINT64 increment) {
    m_pointer = m_pointer + increment > (m_capacity << 3) ? (m_capacity << 3) : m_pointer + increment;
}

void Bitstream8::decrement_pointer(UINT64 decrement) {
    m_pointer = m_pointer - decrement > m_pointer ? 0 : m_pointer - decrement;
}

void Bitstream8::set_pointer(UINT64 index) {
    m_pointer = index > (m_capacity << 3) ? (m_capacity << 3) : index;
}

UINT64 Bitstream8::pointer() {
    return m_pointer;
}

UINT64 Bitstream8::capacity() {
    return m_capacity;
}

void Bitstream8::double_capacity() {
    UINT8* old_buffer = m_bytes;
    m_bytes = new UINT8[m_capacity << 1];
    UINT64 i = 0;
    for(; i < m_capacity; i++) {
        m_bytes[i] = old_buffer[i];
    }
    for(; i < (m_capacity << 1); i++) {
        m_bytes[i] = 0;
    }
    delete[] old_buffer;
    m_capacity <<= 1;
}