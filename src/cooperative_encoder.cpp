#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <error.h>

#include "cooperative_encoder.h"

cooperative_encoder::cooperative_encoder() :
    data(NULL),
    data_pos(0),
    data_len(0),
    chunk_len(0),
    chunks_per_peer(0),
    current_peer_chunks(0)
{ }

void cooperative_encoder::init(unsigned char *d, size_t dl, size_t cl, size_t np){
    //Sanity checks
    if(d == NULL)
        error(-1, 0, "Unable to initialize cooperative_encoder with NULL data");
    if(data_len == 0)
        error(-1, 0, "Unable to initialize cooperative_encoder with zero-length data");
    if(chunk_len == 0)
        error(-1, 0, "Unable to initialize cooperative_encoder with zero-length chunks");
    if (num_peers == 0)
        error(-1, 0, "Unable to initialize cooperative_encoder with zero peers");

    data = d;
    data_len = dl;
    chunk_len = cl;
    num_peers = np;

    double num_chunks = 1.0 * data_len / chunk_len;
    chunks_per_peer = (size_t) ceil(num_chunks / num_peers);
}

int cooperative_encoder::generate_chunk(unsigned char **dest){
    unsigned char *chunk = NULL;
    size_t len = data_len - data_pos;
    if(len > chunk_len) len = chunk_len;

    if(len <= 0 || current_peer_chunks >= chunks_per_peer) {
        *dest = NULL;
        return 0;
    }

    chunk = (unsigned char *)calloc(len, sizeof(unsigned char));
    memset(chunk, 0, len);
    memcpy(chunk, data + data_pos, len);
    data_pos += len;

    current_peer_chunks++;
    *dest = chunk;
    return len;
}

void cooperative_encoder::next_stream() {
    current_peer_chunks = 0;
}
