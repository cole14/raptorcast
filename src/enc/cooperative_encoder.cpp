#include <error.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "cooperative_encoder.h"

Cooperative_Encoder::Cooperative_Encoder(Encoder_Context *ctx) :
    Encoder(ctx)
{
    chunks_per_peer = (size_t) ceil(1.0 * context->get_num_blocks() / context->get_num_peers());
}

std::vector<unsigned> *Cooperative_Encoder::get_chunk_list(unsigned peer) {
    std::vector<unsigned> *chunk_list = new std::vector<unsigned>();

    if (peer < 0 || peer >= context->get_num_peers()) {
        return chunk_list;
    }

    // Figure out what chunks we need
    for (unsigned i = chunks_per_peer * peer; i < chunks_per_peer * (peer+1); i++) {
        unsigned chunk_id = (i % context->get_num_blocks());
        chunk_list->push_back(chunk_id);
    }

    return chunk_list;
}

size_t Cooperative_Encoder::get_chunk (unsigned char **dest, unsigned chunk_id) {
    unsigned char *block = context->get_block(chunk_id);
    if (block == NULL) {
        *dest = NULL;
        return 0;
    }

    unsigned char * chunk = (unsigned char *) malloc(context->get_block_len());
    memcpy(chunk, block, context->get_block_len());
    *dest = chunk;
    return context->get_block_len();
}
