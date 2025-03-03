#include "correct/convolutional/bit.h"

bit_writer_t *bit_writer_create(uint8_t *bytes, size_t len) {
    bit_writer_t *w = (bit_writer_t *)calloc(1, sizeof(bit_writer_t));
    if (!w) {
        return NULL;
    }

    if (bytes) {
        bit_writer_reconfigure(w, bytes, len);
    }

    return w;
}

void bit_writer_reconfigure(bit_writer_t *w, uint8_t *bytes, size_t len) {
    w->bytes = bytes;
    w->len = len;

    w->current_byte = 0;
    w->current_byte_len = 0;
    w->byte_index = 0;
}

void bit_writer_destroy(bit_writer_t *w) {
    if (w) {
        free(w);
    }
}

void bit_writer_write(bit_writer_t *w, uint8_t val, unsigned int n) {
    for (unsigned int j = 0; j < n; j++) {
        bit_writer_write_1(w, val);
        val >>= 1;
    }
}

void bit_writer_write_1(bit_writer_t *w, uint8_t val) {
    w->current_byte |= val & 1;
    w->current_byte_len++;

    if (w->current_byte_len == 8) {
        // 8 bits in a byte -- move to the next byte
        w->bytes[w->byte_index] = w->current_byte;
        w->byte_index++;
        w->current_byte_len = 0;
        w->current_byte = 0;
    } else {
        w->current_byte <<= 1;
    }
}

void bit_writer_write_bitlist(bit_writer_t *w, uint8_t *l, size_t len) {
    if (!w || !l || !w->bytes) {
        return;
    }

    // Check if we have enough space in the destination buffer
    size_t remaining_space = w->len - w->byte_index;
    size_t bits_needed = len + w->current_byte_len;
    size_t bytes_needed = (bits_needed + 7) / 8;

    if (bytes_needed > remaining_space) {
        return;
    }

    // First close the current byte if needed
    size_t close_len = 8 - w->current_byte_len;
    close_len = (close_len < len) ? close_len : len;

    uint16_t b = w->current_byte;

    for (size_t i = 0; i < close_len; i++) {
        if (i >= len) {
            break;
        }

        b |= l[i];
        b <<= 1;
    }

    l += close_len;
    if (len >= close_len) {
        len -= close_len;
    } else {
        len = 0;
    }

    uint8_t *bytes = w->bytes;
    size_t byte_index = w->byte_index;

    if (w->current_byte_len + close_len == 8) {
        if (byte_index < w->len) {
            b >>= 1;
            bytes[byte_index] = (uint8_t)(b & 0xFF);
            byte_index++;
        }
    } else {
        w->current_byte = (uint8_t)(b & 0xFF);
        w->current_byte_len += (unsigned int)close_len;
        return;
    }

    size_t full_bytes = len / 8;

    // Ensure we don't write beyond buffer bounds
    if (byte_index + full_bytes > w->len) {
        full_bytes = w->len - byte_index;
    }

    for (size_t i = 0; i < full_bytes; i++) {
        if (byte_index < w->len) {
            bytes[byte_index] = (uint8_t)((l[0] << 7) | (l[1] << 6) | (l[2] << 5) |
                                        (l[3] << 4) | (l[4] << 3) | (l[5] << 2) |
                                        (l[6] << 1) | l[7]);
            byte_index++;
            l += 8;
        }
    }

    len -= 8 * full_bytes;

    // Handle remaining bits
    b = 0;
    for (size_t i = 0; i < len && i < 8; i++) {
        b |= l[i];
        b <<= 1;
    }

    w->current_byte = (uint8_t)(b & 0xFF);
    w->byte_index = byte_index;
    w->current_byte_len = (len > UINT_MAX) ? UINT_MAX : (unsigned int)len;
}

void bit_writer_write_bitlist_reversed(bit_writer_t *w, uint8_t *l, size_t len) {
    if (!w || !l || !w->bytes) {
        return;
    }

    size_t remaining_space = w->len - w->byte_index;
    size_t bits_needed = len + w->current_byte_len;
    size_t bytes_needed = (bits_needed + 7) / 8;

    if (bytes_needed > remaining_space) {
        return;
    }

    l = l + len - 1;
    uint8_t *bytes = w->bytes;
    size_t byte_index = w->byte_index;
    uint16_t b;

    if (w->current_byte_len != 0) {
        size_t close_len = 8 - w->current_byte_len;
        close_len = (close_len < len) ? close_len : len;

        b = w->current_byte;

        for (size_t i = 0; i < close_len; i++) {
            if (l < l - len + 1) {
                break;
            }

            b |= *l;
            b <<= 1;
            l--;
        }

        len -= close_len;

        if (w->current_byte_len + close_len == 8) {
            if (byte_index < w->len) {
                b >>= 1;
                bytes[byte_index] = (uint8_t)(b & 0xFF);
                byte_index++;
            }
        } else {
            w->current_byte = (uint8_t)(b & 0xFF);
            w->current_byte_len += (unsigned int)close_len;
            return;
        }
    }

    size_t full_bytes = len / 8;

    // Ensure we don't write beyond buffer bounds
    if (byte_index + full_bytes > w->len) {
        full_bytes = w->len - byte_index;
    }

    for (size_t i = 0; i < full_bytes; i++) {
        if (byte_index < w->len) {
            bytes[byte_index] = (uint8_t)((l[0] << 7) | (l[-1] << 6) | (l[-2] << 5) |
                                        (l[-3] << 4) | (l[-4] << 3) | (l[-5] << 2) |
                                        (l[-6] << 1) | l[-7]);
            byte_index++;
            l -= 8;
        }
    }

    len -= 8 * full_bytes;

    b = 0;
    for (size_t i = 0; i < len && i < 8; i++) {
        if (l >= l - len + 1) {
            b |= *l;
            b <<= 1;
            l--;
        }
    }

    w->current_byte = (uint8_t)b;
    w->byte_index = byte_index;
    w->current_byte_len = (len > UINT_MAX) ? UINT_MAX : (unsigned int)len;
}

void bit_writer_flush_byte(bit_writer_t *w) {
    if (w->current_byte_len != 0) {
        w->current_byte <<= (8 - w->current_byte_len);
        w->bytes[w->byte_index] = w->current_byte;
        w->byte_index++;
        w->current_byte_len = 0;
    }
}

size_t bit_writer_length(bit_writer_t *w) {
    return w->byte_index;
}

uint8_t reverse_byte(uint8_t b) {
    return (uint8_t)(((b & 0x80) >> 7) | ((b & 0x40) >> 5) | ((b & 0x20) >> 3) |
           ((b & 0x10) >> 1) | ((b & 0x08) << 1) | ((b & 0x04) << 3) |
           ((b & 0x02) << 5) | ((b & 0x01) << 7));
}

static uint8_t reverse_table[256];

void create_reverse_table(void) {
    for (size_t i = 0; i < 256; i++) {
        reverse_table[i] = reverse_byte((uint8_t)i);
    }
}

bit_reader_t *bit_reader_create(const uint8_t *bytes, size_t len) {
    bit_reader_t *r = (bit_reader_t *)calloc(1, sizeof(bit_reader_t));
    if (!r) {
        return NULL;
    }

    static bool reverse_table_created = false;

    if (!reverse_table_created) {
        create_reverse_table();
        reverse_table_created = true;
    }

    if (bytes) {
        bit_reader_reconfigure(r, bytes, len);
    }

    return r;
}

void bit_reader_reconfigure(bit_reader_t *r, const uint8_t *bytes, size_t len) {
    r->bytes = bytes;
    r->len = len;

    r->current_byte_len = 8;
    r->current_byte = bytes[0];
    r->byte_index = 0;
}

void bit_reader_destroy(bit_reader_t *r) {
    if (r) {
        free(r);
    }
}

uint8_t bit_reader_read(bit_reader_t *r, size_t n) {
    unsigned int read = 0;
    size_t n_copy = n;

    if (r->current_byte_len < n) {
        read = r->current_byte & ((1U << r->current_byte_len) - 1);
        r->byte_index++;
        r->current_byte = r->bytes[r->byte_index];
        n -= r->current_byte_len;
        r->current_byte_len = 8;
        read <<= n;
    }

    uint8_t copy_mask = (uint8_t)((1U << n) - 1);
    copy_mask = (uint8_t)(copy_mask << (r->current_byte_len - n));
    read |= (r->current_byte & copy_mask) >> (r->current_byte_len - n);
    r->current_byte_len -= n;
    return (uint8_t)(reverse_table[read] >> (8 - n_copy));
}
