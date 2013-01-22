#include <stdlib.h>
#include <string.h>

#include "client_server_decoder.h"

void Client_Server_Decoder::notify (unsigned chunk_id) {
    unsigned char *chunk;
    unsigned char *block;
    block = (unsigned char *)malloc(context->get_descriptor()->chunk_len);
    chunk = context->get_chunk(chunk_id);
    memcpy(block, chunk, context->get_descriptor()->chunk_len);
    context->set_block(block, chunk_id);
}

bool Client_Server_Decoder::is_ready() {
    size_t expected_blocks = context->get_descriptor()->total_chunks;
    size_t decoded_blocks = context->get_block_list()->size();
    return decoded_blocks >= expected_blocks;
}
