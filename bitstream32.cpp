#include "Bitstream32.h"
using namespace ezb;

Bitstream32::Bitstream32(UINT64 no_bits) {
    m_pointer = 0;
    m_capacity = (no_bits >> 5) == 0 ? 1 : (no_bits >> 5);
    m_four_bytes = new UINT32[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_four_bytes[i] = 0;
    }
}

Bitstream32::~Bitstream32() {
    delete m_four_bytes;
}

Bitstream32::Bitstream32(const Bitstream32 &other) {
    m_pointer = other.m_pointer;
    m_capacity = other.m_capacity;
    m_four_bytes = new UINT32[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_four_bytes[i] = other.m_four_bytes[i];
    }
}

Bitstream32 Bitstream32::operator=(const Bitstream32 &other) {
    m_pointer = other.m_pointer;
    m_capacity = other.m_capacity;
    m_four_bytes = new UINT32[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_four_bytes[i] = other.m_four_bytes[i];
    }
    return *this;
}

void Bitstream32::set_bit(UINT64 idx) {
    m_four_bytes[(idx >> 5)] |= 0b1ull << (idx & 0b11111ull);
}

void Bitstream32::clear_bit(UINT64 idx) {
    m_four_bytes[(idx >> 5)] &= ~(0b1ull << ((idx & 0b11111ull)));
}

bool Bitstream32::get_bit(UINT64 idx) {
    return (m_four_bytes[(idx >> 5)] & (0b1ull << (idx & 0b11111ull))) > 0;
}

UINT32 Bitstream32::read_word(UINT64 start, UINT8 no_bits_to_read) {
    UINT64 word_idx = start >> 5;
    UINT64 bit_start_offset = start & 0b11111ull;
    UINT64 bit_end_offset = (no_bits_to_read - start + 1) & 0b11111ull;
    if (bit_start_offset == 0) { // word aligned read of no_bits_to_read many bits
        return m_four_bytes[word_idx] & MASK_SHIFT_32_RIGHT[32-no_bits_to_read];
    }
    if (bit_end_offset >= bit_start_offset) { // end and start are in the same word, extract and return middle bits
        return (m_four_bytes[word_idx] & (MASK_SHIFT_32_LEFT[bit_start_offset] & MASK_SHIFT_32_RIGHT[31 - bit_end_offset])) >> bit_start_offset;
    }
    // else, the read is split on two words: read the relevant section from both and combine
    return  ((m_four_bytes[word_idx] & MASK_SHIFT_32_LEFT[bit_start_offset]) >> (bit_start_offset)) |
            ((m_four_bytes[word_idx + 1] & MASK_SHIFT_32_RIGHT[31 - bit_end_offset]) << (32-bit_end_offset));
}

UINT32 Bitstream32::read_word(UINT8 no_bits_to_read) {
    UINT64 word_idx = m_pointer >> 5;
    UINT64 bit_start_offset = m_pointer & 0b11111ull;
    UINT64 bit_end_offset = (no_bits_to_read - m_pointer + 1) & 0b11111ull;
    if (bit_start_offset == 0) { // word aligned read of no_bits_to_read many bits
        return m_four_bytes[word_idx] & MASK_SHIFT_32_RIGHT[32-no_bits_to_read];
    }
    if (bit_end_offset >= bit_start_offset) { // end and start are in the same word, extract and return middle bits
        return (m_four_bytes[word_idx] & (MASK_SHIFT_32_LEFT[bit_start_offset] & MASK_SHIFT_32_RIGHT[31 - bit_end_offset])) >> bit_start_offset;
    }
    // else, the read is split on two words: read the relevant section from both and combine
    return  ((m_four_bytes[word_idx] & MASK_SHIFT_32_LEFT[bit_start_offset]) >> (bit_start_offset)) |
            ((m_four_bytes[word_idx + 1] & MASK_SHIFT_32_RIGHT[31 - bit_end_offset]) << (32-bit_end_offset));
}

void Bitstream32::write_word(UINT64 start, UINT32 data, UINT8 no_bits_to_write) {
    while(start + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    UINT64 word_idx = start >> 5;
    UINT64 bit_start_offset = start & 0b11111ull;
    UINT64 bit_end_offset = (start + no_bits_to_write - 1) & 0b11111ull;

    if (bit_start_offset == 0) { // word aligned write, write no_bits_to_write many bits from data
        m_four_bytes[word_idx] &= MASK_SHIFT_32_LEFT[no_bits_to_write];
        m_four_bytes[word_idx] |= data & MASK_SHIFT_32_RIGHT[32 - no_bits_to_write];
        return;
    }

    if(bit_end_offset >= bit_start_offset) { // write into a single word, clear the middle bits
        UINT32 mask = MASK_SHIFT_32_LEFT[bit_start_offset] & MASK_SHIFT_32_RIGHT[31-bit_end_offset];
        m_four_bytes[word_idx] &= ~mask;
        m_four_bytes[word_idx] |= mask & (data << (bit_start_offset));
        return;
    }
    // write to two adjacent words
    m_four_bytes[word_idx] &= ~MASK_SHIFT_32_LEFT[bit_start_offset];
    m_four_bytes[word_idx++] |= (data << bit_start_offset);
    m_four_bytes[word_idx] &= MASK_SHIFT_32_LEFT[++bit_end_offset];
    m_four_bytes[word_idx] |= (data >> (32 - bit_start_offset)) & ~MASK_SHIFT_32_LEFT[bit_end_offset];
}

void Bitstream32::write_word(UINT32 data, UINT8 no_bits_to_write) {
    while(m_pointer + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    UINT64 word_idx = m_pointer >> 5;
    UINT64 bit_start_offset = m_pointer & 0b11111ull;
    UINT64 bit_end_offset = (m_pointer + no_bits_to_write - 1) & 0b11111ull;

    if (bit_start_offset == 0) { // word aligned write, write no_bits_to_write many bits from data
        m_four_bytes[word_idx] &= MASK_SHIFT_32_LEFT[no_bits_to_write];
        m_four_bytes[word_idx] |= data & MASK_SHIFT_32_RIGHT[32 - no_bits_to_write];
        m_pointer += no_bits_to_write;
        return;
    }
    if(bit_end_offset >= bit_start_offset) { // write into a single word, clear the middle bits
        UINT32 mask = MASK_SHIFT_32_LEFT[bit_start_offset] & MASK_SHIFT_32_RIGHT[31-bit_end_offset];
        m_four_bytes[word_idx] &= ~mask;
        m_four_bytes[word_idx] |= mask & (data << (bit_start_offset));
        m_pointer += no_bits_to_write;
        return;
    }
    // write to two adjacent words
    m_four_bytes[word_idx] &= ~MASK_SHIFT_32_LEFT[bit_start_offset];
    m_four_bytes[word_idx++] |= (data << bit_start_offset);
    m_four_bytes[word_idx] &= MASK_SHIFT_32_LEFT[++bit_end_offset];
    m_four_bytes[word_idx] |= (data >> (32 - bit_start_offset)) & ~MASK_SHIFT_32_LEFT[bit_end_offset];
    m_pointer += no_bits_to_write;
}

void Bitstream32::write_buffer(UINT64 start, UINT32* data, UINT64 data_size, UINT64 no_bits_to_write) {
    while(start + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    UINT64 bit_offset = start & 0b11111ull;
    if (bit_offset) { // unaligned write to the bitstream
        // write the first no_bits_to_write / 32 words to the bitstream
        UINT64 cur_word = start >> 5;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write << 5); i++) {
            m_four_bytes[cur_word] &= ~MASK_SHIFT_32_LEFT[bit_offset];
            m_four_bytes[cur_word++] |= (data[i] << bit_offset);
            m_four_bytes[cur_word] &= MASK_SHIFT_32_LEFT[bit_offset - 1];
            m_four_bytes[cur_word] |= (data[i] >> (32 - bit_offset)) & ~MASK_SHIFT_32_LEFT[bit_offset];
        }
        // write remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset + bits_left - 1) & 0b11111ull;
            if (bit_offset <= end_offset) {
                UINT32 mask = MASK_SHIFT_32_LEFT[bit_offset] & MASK_SHIFT_32_RIGHT[31-end_offset];
                m_four_bytes[cur_word] &= ~mask;
                m_four_bytes[cur_word] |= mask & (data[i] << (bit_offset));
                return;
            } else {
                // unaligned write split on the first and second word
                m_four_bytes[cur_word] &= ~MASK_SHIFT_32_LEFT[bit_offset];
                m_four_bytes[cur_word++] |= (data[i] << bit_offset);
                m_four_bytes[cur_word] &= MASK_SHIFT_32_LEFT[++end_offset];
                m_four_bytes[cur_word] |= (data[i] >> (32 - bit_offset)) & ~MASK_SHIFT_32_LEFT[end_offset];
            }
        }
    } else { // aligned write on the bitstream
        UINT64 cur_word = start >> 5;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write >> 5); i++) {
            m_four_bytes[cur_word++] = data[i];
        }
        // write the remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            m_four_bytes[cur_word] &= MASK_SHIFT_32_LEFT[bits_left];
            m_four_bytes[cur_word] |= data[i] & MASK_SHIFT_32_RIGHT[32 - bits_left];
        }
    }
}

void Bitstream32::write_buffer(UINT32* data, UINT64 data_size, UINT64 no_bits_to_write) {
    while(m_pointer + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    UINT64 bit_offset = m_pointer & 0b11111ull;
    if (bit_offset) { // unaligned write to the bitstream
        // write the first no_bits_to_write / 32 words to the bitstream
        UINT64 cur_word = m_pointer >> 5;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write << 5); i++) {
            m_four_bytes[cur_word] &= ~MASK_SHIFT_32_LEFT[bit_offset];
            m_four_bytes[cur_word++] |= (data[i] << bit_offset);
            m_four_bytes[cur_word] &= MASK_SHIFT_32_LEFT[bit_offset - 1];
            m_four_bytes[cur_word] |= (data[i] >> (32 - bit_offset)) & ~MASK_SHIFT_32_LEFT[bit_offset];
        }
        // write remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset + bits_left - 1) & 0b11111ull;
            if (bit_offset <= end_offset) {
                UINT32 mask = MASK_SHIFT_32_LEFT[bit_offset] & MASK_SHIFT_32_RIGHT[31-end_offset];
                m_four_bytes[cur_word] &= ~mask;
                m_four_bytes[cur_word] |= mask & (data[i] << (bit_offset));
                return;
            } else {
                // unaligned write split on the first and second word
                m_four_bytes[cur_word] &= ~MASK_SHIFT_32_LEFT[bit_offset];
                m_four_bytes[cur_word++] |= (data[i] << bit_offset);
                m_four_bytes[cur_word] &= MASK_SHIFT_32_LEFT[++end_offset];
                m_four_bytes[cur_word] |= (data[i] >> (32 - bit_offset)) & ~MASK_SHIFT_32_LEFT[end_offset];
            }
        }
    } else { // aligned write on the bitstream
        UINT64 cur_word = m_pointer >> 5;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write >> 5); i++) {
            m_four_bytes[cur_word++] = data[i];
        }
        // write the remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            m_four_bytes[cur_word] &= MASK_SHIFT_32_LEFT[bits_left];
            m_four_bytes[cur_word] |= data[i] & MASK_SHIFT_32_RIGHT[32 - bits_left];
        }
    }
    m_pointer += no_bits_to_write;
}

void Bitstream32::write_stream(UINT64 start_destination, UINT64 start_source, UINT64 no_bits_to_write, Bitstream32 &source) {
    if(start_source + no_bits_to_write > (source.m_capacity << 5)) { // reading from unallocated memory
        return;
    }
    while(start_destination + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    // try to word align the source by writing 32 - bit_offset_src bits
    UINT64 initial_bits_to_write = no_bits_to_write < 32 - (start_source & 0b11111ull) ? no_bits_to_write : 32 - no_bits_to_write;
    UINT32 initial_source_data = source.read_word(start_source, initial_bits_to_write);
    write_word(start_destination, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    start_destination += initial_bits_to_write;
    start_source += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = start_destination & 0b11111ull;
    //UINT64 bit_offset_src = start_source & 0b11111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_word_dst = start_destination >> 5;
        UINT64 cur_word_src = start_source >> 5;
        for (i = 0; i < (no_bits_to_write >> 5); i++) { // just copy the words until boundary
            m_four_bytes[cur_word_dst++] = source.m_four_bytes[cur_word_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if(bits_left) { // write the remaining bits to src
            m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[bits_left];
            m_four_bytes[cur_word_dst] |= source.m_four_bytes[cur_word_src] & MASK_SHIFT_32_RIGHT[32 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_word_dst = start_destination >> 5;
        UINT64 cur_word_src = start_source >> 5;
        for(i = 0; i < (no_bits_to_write >> 5); i++) { // writes will always span two words
            m_four_bytes[cur_word_dst] &= ~MASK_SHIFT_32_LEFT[bit_offset_dst];
            m_four_bytes[cur_word_dst++] |= (source.m_four_bytes[cur_word_src] << bit_offset_dst);
            m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[bit_offset_dst - 1];
            m_four_bytes[cur_word_dst] |= (source.m_four_bytes[cur_word_src++] >> (32 - bit_offset_dst)) & ~MASK_SHIFT_32_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b11111ull;
            if (bit_offset_dst <= end_offset) {
                UINT32 mask = MASK_SHIFT_32_LEFT[bit_offset_dst] & MASK_SHIFT_32_RIGHT[31-end_offset];
                m_four_bytes[cur_word_dst] &= ~mask;
                m_four_bytes[cur_word_dst] |= mask & (source.m_four_bytes[cur_word_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second word
                m_four_bytes[cur_word_dst] &= ~MASK_SHIFT_32_LEFT[bit_offset_dst];
                m_four_bytes[cur_word_dst++] |= (source.m_four_bytes[cur_word_src]  << bit_offset_dst);
                m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[++end_offset];
                m_four_bytes[cur_word_dst] |= (source.m_four_bytes[cur_word_src] >> (32 - bit_offset_dst)) & ~MASK_SHIFT_32_LEFT[end_offset];
            }
        }
    }
}

void Bitstream32::write_stream(UINT64 start_source, UINT64 no_bits_to_write, Bitstream32 &source) {
    if(start_source + no_bits_to_write > (source.m_capacity << 5)) { // reading from unallocated memory
        return;
    }
    while(m_pointer + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    // try to word align the source by writing 32 - bit_offset_src bits
    UINT64 initial_bits_to_write = no_bits_to_write < 32 - (start_source & 0b11111ull) ? no_bits_to_write : 32 - no_bits_to_write;
    UINT32 initial_source_data = source.read_word(start_source, initial_bits_to_write);
    write_word(m_pointer, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    m_pointer += initial_bits_to_write;
    start_source += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = m_pointer & 0b11111ull;
    //UINT64 bit_offset_src = start_source & 0b11111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 5;
        UINT64 cur_word_src = start_source >> 5;
        for (i = 0; i < (no_bits_to_write >> 5); i++) { // just copy the words until boundary
            m_four_bytes[cur_word_dst++] = source.m_four_bytes[cur_word_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if(bits_left) { // write the remaining bits to src
            m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[bits_left];
            m_four_bytes[cur_word_dst] |= source.m_four_bytes[cur_word_src] & MASK_SHIFT_32_RIGHT[32 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 5;
        UINT64 cur_word_src = start_source >> 5;
        for(i = 0; i < (no_bits_to_write >> 5); i++) { // writes will always span two words
            m_four_bytes[cur_word_dst] &= ~MASK_SHIFT_32_LEFT[bit_offset_dst];
            m_four_bytes[cur_word_dst++] |= (source.m_four_bytes[cur_word_src] << bit_offset_dst);
            m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[bit_offset_dst - 1];
            m_four_bytes[cur_word_dst] |= (source.m_four_bytes[cur_word_src++] >> (32 - bit_offset_dst)) & ~MASK_SHIFT_32_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b11111ull;
            if (bit_offset_dst <= end_offset) {
                UINT32 mask = MASK_SHIFT_32_LEFT[bit_offset_dst] & MASK_SHIFT_32_RIGHT[31-end_offset];
                m_four_bytes[cur_word_dst] &= ~mask;
                m_four_bytes[cur_word_dst] |= mask & (source.m_four_bytes[cur_word_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second word
                m_four_bytes[cur_word_dst] &= ~MASK_SHIFT_32_LEFT[bit_offset_dst];
                m_four_bytes[cur_word_dst++] |= (source.m_four_bytes[cur_word_src]  << bit_offset_dst);
                m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[++end_offset];
                m_four_bytes[cur_word_dst] |= (source.m_four_bytes[cur_word_src] >> (32 - bit_offset_dst)) & ~MASK_SHIFT_32_LEFT[end_offset];
            }
        }
    }
    m_pointer += no_bits_to_write;
}

void Bitstream32::write_stream(UINT64 no_bits_to_write, Bitstream32 &source) {
    if(source.m_pointer + no_bits_to_write > (source.m_capacity << 5)) { // reading from unallocated memory
        return;
    }
    while(m_pointer + no_bits_to_write > (m_capacity << 5)) {
        double_capacity();
    }
    // try to word align the source by writing 32 - source.m_pointer bits
    UINT64 initial_bits_to_write = no_bits_to_write < 32 - (source.m_pointer & 0b11111ull) ? no_bits_to_write : 32 - no_bits_to_write;
    UINT32 initial_source_data = source.read_word(source.m_pointer, initial_bits_to_write);
    write_word(m_pointer, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    m_pointer += initial_bits_to_write;
    source.m_pointer += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = m_pointer & 0b11111ull;
    //UINT64 bit_offset_src = start_source & 0b11111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 5;
        UINT64 cur_word_src = source.m_pointer >> 5;
        for (i = 0; i < (no_bits_to_write >> 5); i++) { // just copy the words until boundary
            m_four_bytes[cur_word_dst++] = source.m_four_bytes[cur_word_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if(bits_left) { // write the remaining bits to src
            m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[bits_left];
            m_four_bytes[cur_word_dst] |= source.m_four_bytes[cur_word_src] & MASK_SHIFT_32_RIGHT[32 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 5;
        UINT64 cur_word_src = source.m_pointer >> 5;
        for(i = 0; i < (no_bits_to_write >> 5); i++) { // writes will always span two words
            m_four_bytes[cur_word_dst] &= ~MASK_SHIFT_32_LEFT[bit_offset_dst];
            m_four_bytes[cur_word_dst++] |= (source.m_four_bytes[cur_word_src] << bit_offset_dst);
            m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[bit_offset_dst - 1];
            m_four_bytes[cur_word_dst] |= (source.m_four_bytes[cur_word_src++] >> (32 - bit_offset_dst)) & ~MASK_SHIFT_32_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b11111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b11111ull;
            if (bit_offset_dst <= end_offset) {
                UINT32 mask = MASK_SHIFT_32_LEFT[bit_offset_dst] & MASK_SHIFT_32_RIGHT[31-end_offset];
                m_four_bytes[cur_word_dst] &= ~mask;
                m_four_bytes[cur_word_dst] |= mask & (source.m_four_bytes[cur_word_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second word
                m_four_bytes[cur_word_dst] &= ~MASK_SHIFT_32_LEFT[bit_offset_dst];
                m_four_bytes[cur_word_dst++] |= (source.m_four_bytes[cur_word_src]  << bit_offset_dst);
                m_four_bytes[cur_word_dst] &= MASK_SHIFT_32_LEFT[++end_offset];
                m_four_bytes[cur_word_dst] |= (source.m_four_bytes[cur_word_src] >> (32 - bit_offset_dst)) & ~MASK_SHIFT_32_LEFT[end_offset];
            }
        }
    }
    m_pointer += no_bits_to_write;
    source.m_pointer += no_bits_to_write;
}

void Bitstream32::flush(UINT32* &buffer, UINT64 &size, UINT64 new_capacity) {
    buffer = m_four_bytes;
    size = m_capacity;
    m_four_bytes = new UINT32[new_capacity];
    m_capacity = new_capacity;
}

void Bitstream32::increment_pointer(UINT64 increment) {
    m_pointer = m_pointer + increment > (m_capacity << 5) ? (m_capacity << 5) : m_pointer + increment;
}

void Bitstream32::decrement_pointer(UINT64 decrement) {
    m_pointer = m_pointer - decrement > m_pointer ? 0 : m_pointer - decrement;
}

void Bitstream32::set_pointer(UINT64 index) {
    m_pointer = index > (m_capacity << 5) ? (m_capacity << 5) : index;
}

UINT64 Bitstream32::pointer() {
    return m_pointer;
}

UINT64 Bitstream32::capacity() {
    return m_capacity;
}

void Bitstream32::double_capacity() {
    UINT32* old_buffer = m_four_bytes;
    m_four_bytes = new UINT32[m_capacity << 1];
    UINT64 i = 0;
    for(; i < m_capacity; i++) {
        m_four_bytes[i] = old_buffer[i];
    }
    for(; i < (m_capacity << 1); i++) {
        m_four_bytes[i] = 0;
    }
    delete[] old_buffer;
    m_capacity <<= 1;
}