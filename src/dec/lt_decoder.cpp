#include <error.h>
#include <errno.h>
#include <functional>
#include <queue>
#include <stdlib.h>
#include <string.h>

#include "lt_decoder.h"
#include "logger.h"

LT_Decoder::LT_Decoder() :
    msg_desc(NULL),
    chunks_seen(0),
    ready(false)
{ }

LT_Decoder::~LT_Decoder()
{ }

void LT_Decoder::notify(unsigned chunk_id) {
    // First we need to do some setup: build the selector, etc.
    // Requirement: context has already recieved and processed the descriptor
    if (msg_desc == NULL)
        error(-1, EIO, "Recieved lt chunk %u before reading header", chunk_id);

    if (lts == NULL) {
        Message_Descriptor *msg_desc = context->get_descriptor();
        lts = new lt_selector(msg_desc->seed, msg_desc->total_blocks);
    }

    glob_log.log(3, "Read lt chunk %u!\n", chunk_id);

    Chunk *chunk = new Chunk();
    chunk->id = chunk_id;
    chunk->data = context->get_chunk(chunk_id);
    build_block_list(chunk);
    chunks_seen++;

    for (unsigned int i = 0; i < msg_desc->total_blocks; i++) {
        if (decoded_blocks.find(i) != decoded_blocks.end())
            reduce(chunk, decoded_blocks[i]);
    }

    if (chunk->degree == 1) {
        add_block(chunk);
    } else {
        chunk_list.push_back(chunk);
    }
}

/*
 * Iterate the random number generator up to where we figure
 * out what blocks go into the chunk corresponding to this
 * chunk's id
 */
void LT_Decoder::build_block_list(Chunk *chunk) {
    int num_blocks;
    int *selected_blocks;
    num_blocks = lts->select_blocks(chunk->id, &selected_blocks);

    chunk->block_list.clear();
    for (int i = 0; i < num_blocks; i++) {
        chunk->block_list.push_back((unsigned) selected_blocks[i]);
    }
    chunk->degree = chunk->block_list.size();
}

/*
 * checks if a block is in a chunk's block list, and if so,
 * reduces the chunk by the block.
 */
void LT_Decoder::reduce (Chunk *chunk, Block *block) {
    if (chunk->degree < 2)
        return;

    int match = -1;
    for (unsigned int i = 0; i < chunk->block_list.size(); i++) {
        if (chunk->block_list[i] == block->id) {
            match = i;
            break;
        }
    }

    if (match < 0)
        return;

    glob_log.log(3, "Reducing chunk %d by block %d\n",
            chunk->id, block->id);

    chunk->block_list.erase(chunk->block_list.begin()+match);
    chunk->degree--;
    for (unsigned int pos = 0; pos < msg_desc->chunk_len; pos++) {
        chunk->data[pos] ^= block->data[pos];
    }
}

/*
 * Takes a chunk that has been fully reduced (meaning it has
 *   degree = block_list.size() == 1) and builds a block out
 *   of it.
 */
LT_Decoder::Block *LT_Decoder::chunk_to_block(Chunk *chunk) {
    if (chunk->block_list.size() != 1)
        return NULL;
    Block *block = new Block();
    block->data = chunk->data;
    block->id = chunk->block_list[0];
    glob_log.log(5, "Converting chunk %u into block %u.  Contents:\n%s\n",
            chunk->id, block->id, block->data);
    return block;
}

/*
 * Iterate through the list of unfinished chunks, and see if the
 *   incoming chunk (which will be converted into a block) matches
 *   any.  Note that if it does, we will need to iterate them as well.
 */
void LT_Decoder::add_block (Chunk *in_chunk){
    std::queue< Chunk * > work_queue;
    for (work_queue.push(in_chunk); !work_queue.empty(); work_queue.pop()) {
        Block *block = chunk_to_block(work_queue.front());
        for (unsigned int i = 0; i < chunk_list.size(); i++) {
            Chunk *chunk = chunk_list[i];
            reduce(chunk, block);
            if (chunk->degree == 1)
                work_queue.push(chunk);
        }
        decoded_blocks[block->id] = block;
        clean_chunks();
    }
}

void LT_Decoder::clean_chunks () {
    for (unsigned i = 0; i < chunk_list.size(); i++) {
        if (chunk_list[i]->degree == 1) {
            chunk_list.erase(chunk_list.begin()+i);
        }
    }
}

bool LT_Decoder::is_ready () {
    if (msg_desc == NULL) {
        return false;
    } else if (ready) {
        return true;
    } else if (decoded_blocks.size() >= msg_desc->total_blocks) {
        ready = true;
        glob_log.log(2, "Decoded lt message: %zu blocks in %d chunks (%.02f%% overhead)\n",
                msg_desc->total_blocks, chunks_seen,
                100.0 * (chunks_seen- msg_desc->total_blocks) / msg_desc->total_blocks);
        return true;
    } else {
        return false;
    }
}
bool LT_Decoder::is_finished () {
    return false;
}
unsigned char * LT_Decoder::get_message () {
    if (!is_ready())
        return NULL;

    size_t data_len = get_len();
    unsigned char *data, *dp;
    data = (unsigned char *) malloc(data_len);
    dp = data;

    for (unsigned b = 0; b < decoded_blocks.size(); b++) {
        memcpy(dp, decoded_blocks[b]->data, msg_desc->chunk_len);
        dp += msg_desc->chunk_len;
    }

    return data;
}
size_t LT_Decoder::get_len () {
    if (!is_ready())
        return 0;
    return msg_desc->total_blocks * msg_desc->chunk_len;
}
