#include "Bitstream16.h"
using namespace ezb;

Bitstream16::Bitstream16(UINT64 no_bits) {
    m_pointer = 0;
    m_capacity = (no_bits >> 4) == 0 ? 1 : (no_bits >> 4);
    m_two_bytes = new UINT16[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_two_bytes[i] = 0;
    }
}

Bitstream16::~Bitstream16() {
    delete m_two_bytes;
}

Bitstream16::Bitstream16(const Bitstream16 &other) {
    m_pointer = other.m_pointer;
    m_capacity = other.m_capacity;
    m_two_bytes = new UINT16[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_two_bytes[i] = other.m_two_bytes[i];
    }
}

Bitstream16 Bitstream16::operator=(const Bitstream16 &other) {
    m_pointer = other.m_pointer;
    m_capacity = other.m_capacity;
    m_two_bytes = new UINT16[m_capacity];
    for(UINT64 i = 0; i < m_capacity; i++) {
        m_two_bytes[i] = other.m_two_bytes[i];
    }
    return *this;
}

void Bitstream16::set_bit(UINT64 idx) {
    m_two_bytes[(idx >> 4)] |= 0b1ull << (idx & 0b1111ull);
}

void Bitstream16::clear_bit(UINT64 idx) {
    m_two_bytes[(idx >> 4)] &= ~(0b1ull << ((idx & 0b1111ull)));
}

bool Bitstream16::get_bit(UINT64 idx) {
    return (m_two_bytes[(idx >> 4)] & (0b1ull << (idx & 0b1111ull))) > 0;
}

UINT16 Bitstream16::read_word(UINT64 start, UINT8 no_bits_to_read) {
    UINT64 word_idx = start >> 4;
    UINT64 bit_start_offset = start & 0b1111ull;
    UINT64 bit_end_offset = (no_bits_to_read - start + 1) & 0b1111ull;
    if (bit_start_offset == 0) { // word aligned read of no_bits_to_read many bits
        return m_two_bytes[word_idx] & MASK_SHIFT_16_RIGHT[16-no_bits_to_read];
    }
    if (bit_end_offset >= bit_start_offset) { // end and start are in the same word, extract and return middle bits
        return (m_two_bytes[word_idx] & (MASK_SHIFT_16_LEFT[bit_start_offset] & MASK_SHIFT_16_RIGHT[15 - bit_end_offset])) >> bit_start_offset;
    }
    // else, the read is split on two words: read the relevant section from both and combine
    return  ((m_two_bytes[word_idx] & MASK_SHIFT_16_LEFT[bit_start_offset]) >> (bit_start_offset)) |
            ((m_two_bytes[word_idx + 1] & MASK_SHIFT_16_RIGHT[15 - bit_end_offset]) << (16-bit_end_offset));
}

UINT16 Bitstream16::read_word(UINT8 no_bits_to_read) {
    UINT64 word_idx = m_pointer >> 4;
    UINT64 bit_start_offset = m_pointer & 0b1111ull;
    UINT64 bit_end_offset = (no_bits_to_read - m_pointer + 1) & 0b1111ull;
    if (bit_start_offset == 0) { // word aligned read of no_bits_to_read many bits
        return m_two_bytes[word_idx] & MASK_SHIFT_16_RIGHT[16-no_bits_to_read];
    }
    if (bit_end_offset >= bit_start_offset) { // end and start are in the same word, extract and return middle bits
        return (m_two_bytes[word_idx] & (MASK_SHIFT_16_LEFT[bit_start_offset] & MASK_SHIFT_16_RIGHT[15 - bit_end_offset])) >> bit_start_offset;
    }
    // else, the read is split on two words: read the relevant section from both and combine
    return  ((m_two_bytes[word_idx] & MASK_SHIFT_16_LEFT[bit_start_offset]) >> (bit_start_offset)) |
            ((m_two_bytes[word_idx + 1] & MASK_SHIFT_16_RIGHT[15 - bit_end_offset]) << (16-bit_end_offset));
}

void Bitstream16::write_word(UINT64 start, UINT16 data, UINT8 no_bits_to_write) {
    while(start + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    UINT64 word_idx = start >> 4;
    UINT64 bit_start_offset = start & 0b1111ull;
    UINT64 bit_end_offset = (start + no_bits_to_write - 1) & 0b1111ull;

    if (bit_start_offset == 0) { // word aligned write, write no_bits_to_write many bits from data
        m_two_bytes[word_idx] &= MASK_SHIFT_16_LEFT[no_bits_to_write];
        m_two_bytes[word_idx] |= data & MASK_SHIFT_16_RIGHT[16 - no_bits_to_write];
        return;
    }

    if(bit_end_offset >= bit_start_offset) { // write into a single word, clear the middle bits
        UINT16 mask = MASK_SHIFT_16_LEFT[bit_start_offset] & MASK_SHIFT_16_RIGHT[15-bit_end_offset];
        m_two_bytes[word_idx] &= ~mask;
        m_two_bytes[word_idx] |= mask & (data << (bit_start_offset));
        return;
    }
    // write to two adjacent words
    m_two_bytes[word_idx] &= ~MASK_SHIFT_16_LEFT[bit_start_offset];
    m_two_bytes[word_idx++] |= (data << bit_start_offset);
    m_two_bytes[word_idx] &= MASK_SHIFT_16_LEFT[++bit_end_offset];
    m_two_bytes[word_idx] |= (data >> (16 - bit_start_offset)) & ~MASK_SHIFT_16_LEFT[bit_end_offset];
}

void Bitstream16::write_word(UINT16 data, UINT8 no_bits_to_write) {
    while(m_pointer + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    UINT64 word_idx = m_pointer >> 4;
    UINT64 bit_start_offset = m_pointer & 0b1111ull;
    UINT64 bit_end_offset = (m_pointer + no_bits_to_write - 1) & 0b1111ull;

    if (bit_start_offset == 0) { // word aligned write, write no_bits_to_write many bits from data
        m_two_bytes[word_idx] &= MASK_SHIFT_16_LEFT[no_bits_to_write];
        m_two_bytes[word_idx] |= data & MASK_SHIFT_16_RIGHT[16 - no_bits_to_write];
        m_pointer += no_bits_to_write;
        return;
    }
    if(bit_end_offset >= bit_start_offset) { // write into a single word, clear the middle bits
        UINT16 mask = MASK_SHIFT_16_LEFT[bit_start_offset] & MASK_SHIFT_16_RIGHT[15-bit_end_offset];
        m_two_bytes[word_idx] &= ~mask;
        m_two_bytes[word_idx] |= mask & (data << (bit_start_offset));
        m_pointer += no_bits_to_write;
        return;
    }
    // write to two adjacent words
    m_two_bytes[word_idx] &= ~MASK_SHIFT_16_LEFT[bit_start_offset];
    m_two_bytes[word_idx++] |= (data << bit_start_offset);
    m_two_bytes[word_idx] &= MASK_SHIFT_16_LEFT[++bit_end_offset];
    m_two_bytes[word_idx] |= (data >> (15 - bit_start_offset)) & ~MASK_SHIFT_16_LEFT[bit_end_offset];
    m_pointer += no_bits_to_write;
}

void Bitstream16::write_buffer(UINT64 start, UINT16* data, UINT64 data_size, UINT64 no_bits_to_write) {
    while(start + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    UINT64 bit_offset = start & 0b1111ull;
    if (bit_offset) { // unaligned write to the bitstream
        // write the first no_bits_to_write / 16 words to the bitstream
        UINT64 cur_word = start >> 4;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write << 4); i++) {
            m_two_bytes[cur_word] &= ~MASK_SHIFT_16_LEFT[bit_offset];
            m_two_bytes[cur_word++] |= (data[i] << bit_offset);
            m_two_bytes[cur_word] &= MASK_SHIFT_16_LEFT[bit_offset - 1];
            m_two_bytes[cur_word] |= (data[i] >> (16 - bit_offset)) & ~MASK_SHIFT_16_LEFT[bit_offset];
        }
        // write remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset + bits_left - 1) & 0b1111ull;
            if (bit_offset <= end_offset) {
                UINT16 mask = MASK_SHIFT_16_LEFT[bit_offset] & MASK_SHIFT_16_RIGHT[15-end_offset];
                m_two_bytes[cur_word] &= ~mask;
                m_two_bytes[cur_word] |= mask & (data[i] << (bit_offset));
                return;
            } else {
                // unaligned write split on the first and second word
                m_two_bytes[cur_word] &= ~MASK_SHIFT_16_LEFT[bit_offset];
                m_two_bytes[cur_word++] |= (data[i] << bit_offset);
                m_two_bytes[cur_word] &= MASK_SHIFT_16_LEFT[++end_offset];
                m_two_bytes[cur_word] |= (data[i] >> (16 - bit_offset)) & ~MASK_SHIFT_16_LEFT[end_offset];
            }
        }
    } else { // aligned write on the bitstream
        UINT64 cur_word = start >> 4;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write >> 4); i++) {
            m_two_bytes[cur_word++] = data[i];
        }
        // write the remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            m_two_bytes[cur_word] &= MASK_SHIFT_16_LEFT[bits_left];
            m_two_bytes[cur_word] |= data[i] & MASK_SHIFT_16_RIGHT[16 - bits_left];
        }
    }
}

void Bitstream16::write_buffer(UINT16* data, UINT64 data_size, UINT64 no_bits_to_write) {
    while(m_pointer + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    UINT64 bit_offset = m_pointer & 0b1111ull;
    if (bit_offset) { // unaligned write to the bitstream
        // write the first no_bits_to_write / 16 words to the bitstream
        UINT64 cur_word = m_pointer >> 4;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write << 4); i++) {
            m_two_bytes[cur_word] &= ~MASK_SHIFT_16_LEFT[bit_offset];
            m_two_bytes[cur_word++] |= (data[i] << bit_offset);
            m_two_bytes[cur_word] &= MASK_SHIFT_16_LEFT[bit_offset - 1];
            m_two_bytes[cur_word] |= (data[i] >> (16 - bit_offset)) & ~MASK_SHIFT_16_LEFT[bit_offset];
        }
        // write remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset + bits_left - 1) & 0b1111ull;
            if (bit_offset <= end_offset) {
                UINT16 mask = MASK_SHIFT_16_LEFT[bit_offset] & MASK_SHIFT_16_RIGHT[15-end_offset];
                m_two_bytes[cur_word] &= ~mask;
                m_two_bytes[cur_word] |= mask & (data[i] << (bit_offset));
                return;
            } else {
                // unaligned write split on the first and second word
                m_two_bytes[cur_word] &= ~MASK_SHIFT_16_LEFT[bit_offset];
                m_two_bytes[cur_word++] |= (data[i] << bit_offset);
                m_two_bytes[cur_word] &= MASK_SHIFT_16_LEFT[++end_offset];
                m_two_bytes[cur_word] |= (data[i] >> (16 - bit_offset)) & ~MASK_SHIFT_16_LEFT[end_offset];
            }
        }
    } else { // aligned write on the bitstream
        UINT64 cur_word = m_pointer >> 4;
        UINT64 i;
        for (i = 0; i < (no_bits_to_write >> 4); i++) {
            m_two_bytes[cur_word++] = data[i];
        }
        // write the remaining bits, if any
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            m_two_bytes[cur_word] &= MASK_SHIFT_16_LEFT[bits_left];
            m_two_bytes[cur_word] |= data[i] & MASK_SHIFT_16_RIGHT[16 - bits_left];
        }
    }
    m_pointer += no_bits_to_write;
}

void Bitstream16::write_stream(UINT64 start_destination, UINT64 start_source, UINT64 no_bits_to_write, Bitstream16 &source) {
    if(start_source + no_bits_to_write > (source.m_capacity << 4)) { // reading from unallocated memory
        return;
    }
    while(start_destination + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    // try to word align the source by writing 16 - bit_offset_src bits
    UINT64 initial_bits_to_write = no_bits_to_write < 16 - (start_source & 0b1111ull) ? no_bits_to_write : 16 - no_bits_to_write;
    UINT16 initial_source_data = source.read_word(start_source, initial_bits_to_write);
    write_word(start_destination, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    start_destination += initial_bits_to_write;
    start_source += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = start_destination & 0b1111ull;
    //UINT64 bit_offset_src = start_source & 0b1111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_word_dst = start_destination >> 4;
        UINT64 cur_word_src = start_source >> 4;
        for (i = 0; i < (no_bits_to_write >> 4); i++) { // just copy the words until boundary
            m_two_bytes[cur_word_dst++] = source.m_two_bytes[cur_word_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if(bits_left) { // write the remaining bits to src
            m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[bits_left];
            m_two_bytes[cur_word_dst] |= source.m_two_bytes[cur_word_src] & MASK_SHIFT_16_RIGHT[16 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_word_dst = start_destination >> 4;
        UINT64 cur_word_src = start_source >> 4;
        for(i = 0; i < (no_bits_to_write >> 4); i++) { // writes will always span two words
            m_two_bytes[cur_word_dst] &= ~MASK_SHIFT_16_LEFT[bit_offset_dst];
            m_two_bytes[cur_word_dst++] |= (source.m_two_bytes[cur_word_src] << bit_offset_dst);
            m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[bit_offset_dst - 1];
            m_two_bytes[cur_word_dst] |= (source.m_two_bytes[cur_word_src++] >> (16 - bit_offset_dst)) & ~MASK_SHIFT_16_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b1111ull;
            if (bit_offset_dst <= end_offset) {
                UINT16 mask = MASK_SHIFT_16_LEFT[bit_offset_dst] & MASK_SHIFT_16_RIGHT[15-end_offset];
                m_two_bytes[cur_word_dst] &= ~mask;
                m_two_bytes[cur_word_dst] |= mask & (source.m_two_bytes[cur_word_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second word
                m_two_bytes[cur_word_dst] &= ~MASK_SHIFT_16_LEFT[bit_offset_dst];
                m_two_bytes[cur_word_dst++] |= (source.m_two_bytes[cur_word_src]  << bit_offset_dst);
                m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[++end_offset];
                m_two_bytes[cur_word_dst] |= (source.m_two_bytes[cur_word_src] >> (16 - bit_offset_dst)) & ~MASK_SHIFT_16_LEFT[end_offset];
            }
        }
    }
}

void Bitstream16::write_stream(UINT64 start_source, UINT64 no_bits_to_write, Bitstream16 &source) {
    if(start_source + no_bits_to_write > (source.m_capacity << 4)) { // reading from unallocated memory
        return;
    }
    while(m_pointer + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    // try to word align the source by writing 16 - bit_offset_src bits
    UINT64 initial_bits_to_write = no_bits_to_write < 16 - (start_source & 0b1111ull) ? no_bits_to_write : 16 - no_bits_to_write;
    UINT16 initial_source_data = source.read_word(start_source, initial_bits_to_write);
    write_word(m_pointer, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    m_pointer += initial_bits_to_write;
    start_source += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = m_pointer & 0b1111ull;
    //UINT64 bit_offset_src = start_source & 0b1111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 4;
        UINT64 cur_word_src = start_source >> 4;
        for (i = 0; i < (no_bits_to_write >> 4); i++) { // just copy the words until boundary
            m_two_bytes[cur_word_dst++] = source.m_two_bytes[cur_word_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if(bits_left) { // write the remaining bits to src
            m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[bits_left];
            m_two_bytes[cur_word_dst] |= source.m_two_bytes[cur_word_src] & MASK_SHIFT_16_RIGHT[16 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 4;
        UINT64 cur_word_src = start_source >> 4;
        for(i = 0; i < (no_bits_to_write >> 4); i++) { // writes will always span two words
            m_two_bytes[cur_word_dst] &= ~MASK_SHIFT_16_LEFT[bit_offset_dst];
            m_two_bytes[cur_word_dst++] |= (source.m_two_bytes[cur_word_src] << bit_offset_dst);
            m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[bit_offset_dst - 1];
            m_two_bytes[cur_word_dst] |= (source.m_two_bytes[cur_word_src++] >> (16 - bit_offset_dst)) & ~MASK_SHIFT_16_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b1111ull;
            if (bit_offset_dst <= end_offset) {
                UINT16 mask = MASK_SHIFT_16_LEFT[bit_offset_dst] & MASK_SHIFT_16_RIGHT[15-end_offset];
                m_two_bytes[cur_word_dst] &= ~mask;
                m_two_bytes[cur_word_dst] |= mask & (source.m_two_bytes[cur_word_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second word
                m_two_bytes[cur_word_dst] &= ~MASK_SHIFT_16_LEFT[bit_offset_dst];
                m_two_bytes[cur_word_dst++] |= (source.m_two_bytes[cur_word_src]  << bit_offset_dst);
                m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[++end_offset];
                m_two_bytes[cur_word_dst] |= (source.m_two_bytes[cur_word_src] >> (16 - bit_offset_dst)) & ~MASK_SHIFT_16_LEFT[end_offset];
            }
        }
    }
    m_pointer += no_bits_to_write;
}

void Bitstream16::write_stream(UINT64 no_bits_to_write, Bitstream16 &source) {
    if(source.m_pointer + no_bits_to_write > (source.m_capacity << 4)) { // reading from unallocated memory
        return;
    }
    while(m_pointer + no_bits_to_write > (m_capacity << 4)) {
        double_capacity();
    }
    // try to word align the source by writing 16 - source.m_pointer bits
    UINT64 initial_bits_to_write = no_bits_to_write < 16 - (source.m_pointer & 0b1111ull) ? no_bits_to_write : 16 - no_bits_to_write;
    UINT16 initial_source_data = source.read_word(source.m_pointer, initial_bits_to_write);
    write_word(m_pointer, initial_source_data, initial_bits_to_write); // TODO inline this by hand

    m_pointer += initial_bits_to_write;
    source.m_pointer += initial_bits_to_write;
    no_bits_to_write -= initial_bits_to_write;

    if (!no_bits_to_write) return; // nothing left to write, return

    UINT64 bit_offset_dst = m_pointer & 0b1111ull;
    //UINT64 bit_offset_src = start_source & 0b1111ull; // at this point bit_offset_src is zero

    // at this point bit_offset_src is zero
    if (!bit_offset_dst) { // write is word aligned on both buffers
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 4;
        UINT64 cur_word_src = source.m_pointer >> 4;
        for (i = 0; i < (no_bits_to_write >> 4); i++) { // just copy the words until boundary
            m_two_bytes[cur_word_dst++] = source.m_two_bytes[cur_word_src++];
        }
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if(bits_left) { // write the remaining bits to src
            m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[bits_left];
            m_two_bytes[cur_word_dst] |= source.m_two_bytes[cur_word_src] & MASK_SHIFT_16_RIGHT[16 - bits_left];
        }
    } else { // write is not aligned to the destination, but is aligned to the source
        UINT64 i;
        UINT64 cur_word_dst = m_pointer >> 4;
        UINT64 cur_word_src = source.m_pointer >> 4;
        for(i = 0; i < (no_bits_to_write >> 4); i++) { // writes will always span two words
            m_two_bytes[cur_word_dst] &= ~MASK_SHIFT_16_LEFT[bit_offset_dst];
            m_two_bytes[cur_word_dst++] |= (source.m_two_bytes[cur_word_src] << bit_offset_dst);
            m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[bit_offset_dst - 1];
            m_two_bytes[cur_word_dst] |= (source.m_two_bytes[cur_word_src++] >> (16 - bit_offset_dst)) & ~MASK_SHIFT_16_LEFT[bit_offset_dst];
        }
        UINT64 bits_left = no_bits_to_write & 0b1111ull;
        if (bits_left) {
            UINT64 end_offset = (bit_offset_dst + bits_left - 1) & 0b1111ull;
            if (bit_offset_dst <= end_offset) {
                UINT16 mask = MASK_SHIFT_16_LEFT[bit_offset_dst] & MASK_SHIFT_16_RIGHT[15-end_offset];
                m_two_bytes[cur_word_dst] &= ~mask;
                m_two_bytes[cur_word_dst] |= mask & (source.m_two_bytes[cur_word_src] << (bit_offset_dst));
            } else {
                // unaligned write split on the first and second word
                m_two_bytes[cur_word_dst] &= ~MASK_SHIFT_16_LEFT[bit_offset_dst];
                m_two_bytes[cur_word_dst++] |= (source.m_two_bytes[cur_word_src]  << bit_offset_dst);
                m_two_bytes[cur_word_dst] &= MASK_SHIFT_16_LEFT[++end_offset];
                m_two_bytes[cur_word_dst] |= (source.m_two_bytes[cur_word_src] >> (16 - bit_offset_dst)) & ~MASK_SHIFT_16_LEFT[end_offset];
            }
        }
    }
    m_pointer += no_bits_to_write;
    source.m_pointer += no_bits_to_write;
}

void Bitstream16::flush(UINT16* &buffer, UINT64 &size, UINT64 new_capacity) {
    buffer = m_two_bytes;
    size = m_capacity;
    m_two_bytes = new UINT16[new_capacity];
    m_capacity = new_capacity;
}

void Bitstream16::increment_pointer(UINT64 increment) {
    m_pointer = m_pointer + increment > (m_capacity << 4) ? (m_capacity << 4) : m_pointer + increment;
}

void Bitstream16::decrement_pointer(UINT64 decrement) {
    m_pointer = m_pointer - decrement > m_pointer ? 0 : m_pointer - decrement;
}

void Bitstream16::set_pointer(UINT64 index) {
    m_pointer = index > (m_capacity << 4) ? (m_capacity << 4) : index;
}

UINT64 Bitstream16::pointer() {
    return m_pointer;
}

UINT64 Bitstream16::capacity() {
    return m_capacity;
}

void Bitstream16::double_capacity() {
    UINT16* old_buffer = m_two_bytes;
    m_two_bytes = new UINT16[m_capacity << 1];
    UINT64 i = 0;
    for(; i < m_capacity; i++) {
        m_two_bytes[i] = old_buffer[i];
    }
    for(; i < (m_capacity << 1); i++) {
        m_two_bytes[i] = 0;
    }
    delete[] old_buffer;
    m_capacity <<= 1;
}