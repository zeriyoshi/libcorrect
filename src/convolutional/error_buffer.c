#include "correct/convolutional/error_buffer.h"

void error_buffer_destroy(error_buffer_t *buf) {
    if (!buf) {
        return;
    }

    if (buf->errors[0]) {
        free(buf->errors[0]);
    }

    if (buf->errors[1]) {
        free(buf->errors[1]);
    }

    free(buf);
}

error_buffer_t *error_buffer_create(unsigned int num_states) {
    error_buffer_t *buf = (error_buffer_t *)calloc(1, sizeof(error_buffer_t));
    if (!buf) {
        return NULL;
    }

    // how large are the error buffers?
    buf->num_states = num_states;

    // save two error metrics, one for last round and one for this
    // (double buffer)
    // the error metric is the aggregated number of bit errors found
    //   at a given path which terminates at a particular shift register state
    buf->errors[0] = (distance_t *)calloc(num_states, sizeof(distance_t));
    if (!buf->errors[0]) {
        error_buffer_destroy(buf);
        return NULL;
    }

    buf->errors[1] = (distance_t *)calloc(num_states, sizeof(distance_t));
    if (!buf->errors[1]) {
        error_buffer_destroy(buf);
        return NULL;
    }

    // which buffer are we using, 0 or 1?
    buf->index = 0;

    buf->read_errors = buf->errors[0];
    buf->write_errors = buf->errors[1];

    return buf;
}

void error_buffer_reset(error_buffer_t *buf) {
    memset(buf->errors[0], 0, buf->num_states * sizeof(distance_t));
    memset(buf->errors[1], 0, buf->num_states * sizeof(distance_t));
    buf->index = 0;
    buf->read_errors = buf->errors[0];
    buf->write_errors = buf->errors[1];
}

void error_buffer_swap(error_buffer_t *buf) {
    buf->read_errors = buf->errors[buf->index];
    buf->index = (buf->index + 1) % 2;
    buf->write_errors = buf->errors[buf->index];
}
