#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <error.h>

#include "client_server_encoder.h"

Client_Server_Encoder::Client_Server_Encoder(Encoder_Context *ctx) :
    Encoder(ctx),
    data(NULL),
    data_pos(0),
    data_len(0),
    next_chunk_id(0),
    chunk_len(0)
{ }

std::vector<unsigned> *Client_Server_Encoder::get_chunk_list(unsigned peer) {
    std::vector<unsigned> *chunk_list = new std::vector<unsigned>();

    // For the client-server broadcast, we send every chunk to every peer
    for (unsigned i = 0; i < context->get_num_blocks(); i++) {
        chunk_list->push_back(i);
    }
    return chunk_list;
}

size_t Client_Server_Encoder::get_chunk(unsigned char **dest, unsigned chunk_id) {
    unsigned char *block = context->get_block(chunk_id);
    if (block == NULL) {
        *dest = NULL;
        return 0;
    }

    unsigned char *chunk = (unsigned char *)malloc(context->get_block_len());
    memcpy(chunk, block, context->get_block_len());
    *dest = chunk;
    return context->get_block_len();
}
