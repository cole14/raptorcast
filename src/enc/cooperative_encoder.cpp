#include <error.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cooperative_encoder.h"

Cooperative_Encoder::Cooperative_Encoder() :
    Encoder(ctx)
{
    chunks_per_peer = (size_t) ceil(1.0 * context->get_num_blocks() / context->get_num_peers());
}

std::vector<unsigned> *get_chunk_list(unsigned peer) {
    std::vector<unsigned *chunk_list = new std::vector<unsigned>();

    if (peer < 0 || peer >= context->get_num_peers()) {
        return chunk_list;
    }

    // Always send the descriptor first
    chunk_list.push_back(0);

    // Figure out what other chunks we need
    for (int i = chunks_per_peer * peer; i < chunks_per_peer * (peer+1); i++) {
        // The descriptor is chunk 0, so increment.
        unsigned chunk_id = (i % context->get_num_blocks()) + 1;
        chunk_list.push_back(chunk_id);
    }

    return chunk_list;
}

size_t get_chunk (unsigned char **dest, unsigned chunk_id) {
    if (chunk_id == 0) {
        struct coop_descriptor *desc;
        desc = (struct coop_descriptor *) malloc(sizeof(struct coop_descriptor));
        desc->total_chunks = context->get_num_blocks();
        desc->num_peers = context->get_num_peers();
        desc->chunk_len = context->get_block_len();

        *dest = (unsigned char *)desc;
        return sizeof(struct coop_descriptor);

    } else {
        unsigned char *block = context->get_block(chunk_id - 1);
        if (block == NULL) {
            *dest = NULL;
            return 0;
        }

        unsigned char * chunk = (unsigned char *) malloc(context->get_block_len());
        memcpy(chunk, block, context->get_block_len());
        *dest = chunk;
        return context->get_block_len();
    }
}
