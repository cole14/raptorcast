/*
 * Adapted from the c# implementation by Omar Gamil
 * http://www.codeproject.com/Articles/425456/Your-Digital-Fountain
 */

#include <error.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lt_common.h"
#include "lt_encoder.h"

#define LT_SEED 42

lt_encoder::lt_encoder() :
    data_len(0),
    chunk_len(0),
    num_peers(0),
    chunks_per_peer(0),
    current_peer_chunks(0),
    descriptor_sent(false),
    chunk_id(1)
{ }

void lt_encoder::init(unsigned char *d, size_t dl, size_t cl, size_t np){
    //Sanity checks
    if(d == NULL)
        error(-1, 0, "Unable to initialize lt_encoder with NULL data");
    if(dl == 0)
        error(-1, 0, "Unable to initialize lt_encoder with zero-length data");
    if(cl == 0)
        error(-1, 0, "Unable to initialize lt_encoder with zero-length chunks");
    if (np == 0)
        error(-1, 0, "Unable to initialize lt_encoder with zero peers");

    data_len = dl;
    chunk_len = cl;
    num_peers = np;

    total_chunks = (size_t) ceil(1.0 * data_len / chunk_len);
    total_chunks += 1;  // Account for the descriptor
    chunks_per_peer = (size_t) ceil(2.0 * total_chunks / num_peers);


    // Split up the data into blocks
    split_blocks(d, dl);

    // Prepare the RNG
    lts = new lt_selector(LT_SEED, blocks.size());
}

struct lt_descriptor *lt_encoder::get_desc() {
    lt_descriptor *msg_desc = new lt_descriptor();
    msg_desc->total_blocks = blocks.size();
    msg_desc->num_peers = num_peers;
    msg_desc->total_chunks = total_chunks;
    msg_desc->chunk_len = chunk_len;
    msg_desc->seed = LT_SEED;
    return msg_desc;
}

int lt_encoder::generate_chunk(unsigned char **dest, unsigned int *out_chunk_id){
    // If this is the first chunk for this peer, send the header!
    if (!descriptor_sent) {
        glob_log.log(7, "LT encoder generating descriptor chunk\n");
        descriptor_sent = true;

        lt_descriptor *msg_desc = get_desc();

        *dest = (unsigned char *)msg_desc;
        *out_chunk_id = 0;
        return sizeof(lt_descriptor);
    }

    if (current_peer_chunks >= chunks_per_peer) {
        glob_log.log(7, "LT encoder generating terminator chunk\n");
        *dest = NULL;
        *out_chunk_id = 0;
        return 0;
    }

    // OK, we're good to send out a content chunk.
    glob_log.log(7, "LT encoder generating content chunk %u\n", chunk_id);

    // First, choose which blocks we'll use for this chunk
    int num_blocks;
    int *selected_blocks;
    num_blocks = lts->select_blocks(chunk_id, &selected_blocks);

    // Now, turn those blocks into a chunk
    // Note that the data is padded, so we don't have to worry
    // about reading somewhere bad
    unsigned char * chunk;
    chunk = (unsigned char *) malloc(chunk_len);
    memset(chunk, 0, chunk_len);
    if (num_blocks > 1) {
        // XOR all the blocks together
        // XXX optimize?
        for (int pos = 0; pos < (int) chunk_len; pos++) {
            unsigned char byte = 0;
            for (int i = 0; i < num_blocks; i++) {
                byte ^= blocks[selected_blocks[i]][pos];
            }
            chunk[pos] = byte;
        }

    } else {
        // Easy peasy
        memcpy(chunk, blocks[selected_blocks[0]], chunk_len);
    }

    delete[] selected_blocks;

    current_peer_chunks++;
    *dest = chunk;
    *out_chunk_id = chunk_id++;
    return chunk_len;
}

void lt_encoder::next_stream() {
    current_peer_chunks = 0;
    descriptor_sent = false;
}

/* Generates a vector of pointers to the different blocks in the data */
void lt_encoder::split_blocks(unsigned char *data, size_t data_len) {
    // Pad the data to an integer number of chunks
    size_t padded_size = total_chunks * chunk_len;
    unsigned char *padded_data = (unsigned char *) malloc(padded_size);
    memset(padded_data, 0, padded_size);
    memcpy(padded_data, data, data_len);

    // Split up the data
    unsigned char *b_pos;
    for (b_pos = padded_data; b_pos < padded_data+padded_size; b_pos += chunk_len) {
        blocks.push_back(b_pos);
    }
}

