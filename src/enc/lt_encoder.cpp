/*
 * Adapted from the c# implementation by Omar Gamil
 * http://www.codeproject.com/Articles/425456/Your-Digital-Fountain
 */

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "encoder.h"
#include "lt_common.h"
#include "lt_encoder.h"

#define LT_SEED 42

LT_Encoder::LT_Encoder(Encoder_Context *ctx) :
    Encoder(ctx),
    chunks_per_peer(0)
{
    // Calculate the number of chunks each peer will recieve, excluding the descriptor
    chunks_per_peer = (size_t) ceil(2.0 * context->get_num_blocks() /
            context->get_num_peers());

    // Prepare the RNG
    lts = new lt_selector(LT_SEED, context->get_num_blocks());
}

struct Message_Descriptor *LT_Encoder::get_descriptor() {
    LT_Descriptor *msg_desc = (LT_Descriptor *) Encoder::get_descriptor();
    msg_desc = (LT_Descriptor *) realloc(msg_desc, sizeof(LT_Descriptor));
    msg_desc->total_blocks = context->get_num_blocks();
    msg_desc->seed = LT_SEED;
    return (Message_Descriptor *) msg_desc;
}

std::vector<unsigned> *LT_Encoder::get_chunk_list(unsigned peer) {
    std::vector<unsigned> *chunk_list = new std::vector<unsigned>();

    // Make sure the peer request is in bounds
    if (peer < 0 || peer >= context->get_num_peers()) {
        return chunk_list;
    }

    // Send chunks_per_peer chunks as a contiguous list
    for (unsigned i = chunks_per_peer * peer; i < chunks_per_peer * (peer+1); i++) {
        unsigned chunk_id = i;
        chunk_list->push_back(chunk_id);
    }

    return chunk_list;
}

size_t LT_Encoder::get_chunk(unsigned char **dest, unsigned chunk_id) {
    glob_log.log(7, "LT encoder generating content chunk %u\n", chunk_id);

    // First, choose which blocks we'll use for this chunk
    int num_blocks;
    int *selected_blocks;
    num_blocks = lts->select_blocks(chunk_id, &selected_blocks);

    // Now, turn those blocks into a chunk
    // Note that the data is padded, so we don't have to worry
    // about reading somewhere bad
    unsigned char * chunk;
    chunk = (unsigned char *) malloc(context->get_block_len());
    memset(chunk, 0, context->get_block_len());
    if (num_blocks > 1) {
        // XOR all the blocks together, byte by byte
        // XXX (Dan, 12/14/12) optimize?
        for (int pos = 0; pos < (int) context->get_block_len(); pos++) {
            unsigned char byte = 0;
            for (int i = 0; i < num_blocks; i++) {
                byte ^= context->get_block(selected_blocks[i])[pos];
            }
            chunk[pos] = byte;
        }

    } else {
        // Easy peasy
        memcpy(chunk, context->get_block(selected_blocks[0]), context->get_block_len());
    }

    *dest = chunk;
    return context->get_block_len();
}
