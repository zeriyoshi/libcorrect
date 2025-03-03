#include "correct/convolutional/convolutional.h"

// https://www.youtube.com/watch?v=b3_lVSrPB6w

correct_convolutional *_correct_convolutional_init(correct_convolutional *conv, size_t rate, size_t order, const polynomial_t *poly) {
    if (order >= 8 * sizeof(shift_register_t)) {
        // XXX turn this into an error code
        // printf("order must be smaller than 8 * sizeof(shift_register_t)\n");
        return NULL;
    }

    if (rate < 2) {
        // XXX turn this into an error code
        // printf("rate must be 2 or greater\n");
        return NULL;
    }

    conv->order = order;
    conv->rate = rate;
    conv->numstates = 1ULL << order;

    unsigned int *table = (unsigned int *)malloc(sizeof(unsigned int) * (1ULL << order));
    if (!table) {
        return NULL;
    }

    fill_table((unsigned int)conv->rate, (unsigned int)conv->order, poly, table);
    conv->table = table;

    conv->bit_writer = bit_writer_create(NULL, 0);
    if (!conv->bit_writer) {
        free(conv->table);
        return NULL;
    }

    conv->bit_reader = bit_reader_create(NULL, 0);
    if (!conv->bit_reader) {
        bit_writer_destroy(conv->bit_writer);
        free(conv->table);
        return NULL;
    }

    conv->has_init_decode = false;
    return conv;
}

correct_convolutional *correct_convolutional_create(size_t rate, size_t order, const polynomial_t *poly) {
    correct_convolutional *conv = (correct_convolutional *)malloc(sizeof(correct_convolutional));
    if (!conv) {
        return NULL;
    }

    correct_convolutional *init_conv = _correct_convolutional_init(conv, rate, order, poly);
    if (!init_conv) {
        free(conv);
        return NULL;
    }

    return init_conv;
}

void _correct_convolutional_teardown(correct_convolutional *conv) {
    if (conv->table) {
        free(conv->table);
    }

    if (conv->bit_writer) {
        bit_writer_destroy(conv->bit_writer);
    }

    if (conv->bit_reader) {
        bit_reader_destroy(conv->bit_reader);
    }

    if (conv->has_init_decode) {
        pair_lookup_destroy(conv->pair_lookup);
        history_buffer_destroy(conv->history_buffer);
        error_buffer_destroy(conv->errors);
        free(conv->distances);
    }
}

void correct_convolutional_destroy(correct_convolutional *conv) {
    if (conv) {
        _correct_convolutional_teardown(conv);
        free(conv);    
    }
}
