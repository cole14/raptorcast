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
    if (context->get_descriptor() == NULL)
        return false;

    std::vector<unsigned> block_list;
    context->fill_block_list(&block_list);
    size_t expected_blocks = context->get_descriptor()->total_blocks;
    size_t decoded_blocks = block_list.size();
    return decoded_blocks >= expected_blocks;
}
