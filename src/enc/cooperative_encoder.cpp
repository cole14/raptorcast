#include <error.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cooperative_encoder.h"

cooperative_encoder::cooperative_encoder() :
    data(NULL),
    data_pos(0),
    data_len(0),
    chunk_len(0),
    chunks_per_peer(0),
    current_peer_chunks(0),
    descriptor_sent(false),
    chunk_id(0)
{ }

void cooperative_encoder::init(unsigned char *d, size_t dl, size_t cl, size_t np){
    //Sanity checks
    if(d == NULL)
        error(-1, 0, "Unable to initialize cooperative_encoder with NULL data");
    if(dl == 0)
        error(-1, 0, "Unable to initialize cooperative_encoder with zero-length data");
    if(cl == 0)
        error(-1, 0, "Unable to initialize cooperative_encoder with zero-length chunks");
    if (np == 0)
        error(-1, 0, "Unable to initialize cooperative_encoder with zero peers");

    data = d;
    data_len = dl;
    chunk_len = cl;
    num_peers = np;

    total_chunks = (size_t) ceil(1.0 * data_len / chunk_len);
    total_chunks += 1;  // Account for the descriptor
    chunks_per_peer = (size_t) ceil(1.0 * total_chunks / num_peers);
}

int cooperative_encoder::generate_chunk(unsigned char **dest, unsigned int *out_chunk_id){
    size_t len = 0;
    if (descriptor_sent) {
        unsigned char *chunk = NULL;
        len = data_len - data_pos;
        if(len > chunk_len) len = chunk_len;

        if(len <= 0 || current_peer_chunks >= chunks_per_peer) {
            *dest = NULL;
            return 0;
        }

        chunk = (unsigned char *)calloc(len, sizeof(unsigned char));
        memset(chunk, 0, len);
        memcpy(chunk, data + data_pos, len);
        data_pos += len;

        *dest = chunk;
    } else {
        struct coop_descriptor *desc;
        desc = (struct coop_descriptor *) malloc(sizeof(struct coop_descriptor));
        desc->total_chunks = total_chunks;
        desc->num_peers = num_peers;
        desc->chunk_len = chunk_len;

        *dest = (unsigned char *)desc;
        len = sizeof(struct coop_descriptor);
        descriptor_sent = true;
    }

    current_peer_chunks++;
    *out_chunk_id = chunk_id++;
    return len;
}

void cooperative_encoder::next_stream() {
    current_peer_chunks = 0;
}
