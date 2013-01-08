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

std::vector<int> *Client_Server_Encoder::get_chunk_list(unsigned peer) {
    std::vector<int> *chunk_list = new std::vector<int>();

    // For the client-server broadcast, we send every chunk to every peer
    for (int i = 0; i < ctx->get_num_blocks(); i++) {
        chunk_list->push_back(i);
    }
    return chunk_list;
}

size_t Client_Server_Encoder::get_chunk(unsigned char **dest, unsigned chunk_id) {
    unsigned char *block = ctx->get_block(chunk_id);
    if (chunk == NULL) {
        *dest = NULL;
        return 0;
    }

    unsigned char *chunk = malloc(ctx->get_block_len());
    memcpy(chunk, block, ctx->get_block_len());
    *dest = chunk;
    return ctx->get_block_len();
}
