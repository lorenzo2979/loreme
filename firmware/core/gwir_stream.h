#ifndef GWIR_STREAM_H
#define GWIR_STREAM_H
#include "gwir_protocol.h"

/*
 * Length-safe, incremental GWIR framing over a byte-stream transport (TCP).
 * One stream per authenticated client session. This does NOT implement auth.
 * User-supplied handler is called synchronously with exactly one validated
 * message, and may not retain the borrowed `message` pointer after returning.
 * Feed must be called by ONE serialized networking task, not an ISR.
 */
typedef int (*gwir_stream_handler)(void *context,
                                   const uint8_t *message, size_t size);

typedef struct {
    size_t filled;
    size_t expected; /* 0 until the first 20 header bytes arrive */
    uint32_t messages_ok;
    uint32_t messages_invalid;
    int failed; /* after any parse/callback error, close socket and reset */
    uint8_t buffer[GWIR_MAX_MESSAGE];
} gwir_stream;

void gwir_stream_init(gwir_stream *s);
/* Returns completed message count >=0 or -1 on invalid input/callback failure.
 * On -1, caller MUST terminate that client connection and should not send
 * an application response based on the partial data.
 */
int gwir_stream_feed(gwir_stream *s, const uint8_t *input, size_t length,
                     gwir_stream_handler handler, void *context);

#endif
