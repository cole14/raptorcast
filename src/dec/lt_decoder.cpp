#include <error.h>
#include <errno.h>
#include <functional>
#include <queue>
#include <stdlib.h>
#include <string.h>

#include "lt_decoder.h"
#include "logger.h"
#include "message_types.h"

LT_Decoder::LT_Decoder(Decoder_Context *ctx) :
    Decoder(ctx),
    lts(NULL),
    chunks_seen(0),
    ready(false)
{ }

LT_Decoder::~LT_Decoder()
{
    for (unsigned i = 0; i < chunk_list.size(); i++)
        delete chunk_list[i];

    if (lts) {
        delete lts;
    }
}

void LT_Decoder::notify(unsigned chunk_id) {
    chunks_seen++;
    // First we need to do some setup: build the selector, etc.
    // Requirement: context has already recieved and processed the descriptor
    Message_Descriptor *msg_desc = context->get_descriptor();
    if (msg_desc == NULL)
        error(-1, EIO, "Recieved lt chunk %u before reading header", chunk_id);

    if (lts == NULL) {
        lts = new lt_selector(msg_desc->seed, msg_desc->total_blocks);
    }

    glob_log.log(3, "Read lt chunk %u!\n", chunk_id);

    Chunk *chunk = new Chunk();
    chunk->id = chunk_id;
    chunk->data = context->get_chunk(chunk_id);
    build_block_list(chunk);

    // dblock = decoded block, cblock = chunk block
    std::vector<unsigned> dblock_list;
    std::vector<unsigned>::iterator dblock_it, cblock_it;
    context->fill_block_list(&dblock_list);

    for (dblock_it = dblock_list.begin(); dblock_it != dblock_list.end(); dblock_it++) {
        reduce(chunk, *dblock_it);
    }

    if (chunk->degree == 1) {
        add_block(chunk);
        delete chunk;
    } else {
        chunk_list.push_back(chunk);
    }
}


/*
 * Get the list of blocks for this chunk from the selector
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
 * checks if a block was used to make a chunk, and if so,
 * reduces the chunk by the block.
 */
void LT_Decoder::reduce (Chunk *chunk, unsigned block_id) {
    if (chunk->degree < 2)
        return;

    // Figure out whether this block was used in the making of this chunk
    int match = -1;
    for (unsigned int i = 0; i < chunk->block_list.size(); i++) {
        if (chunk->block_list[i] == block_id) {
            match = i;
            break;
        }
    }

    if (match < 0)
        return;

    glob_log.log(3, "Reducing chunk %d by block %d\n",
            chunk->id, block_id);

    unsigned char *block = context->get_block(block_id);
    for (unsigned pos = 0; pos < context->get_descriptor()->chunk_len; pos++) {
        chunk->data[pos] ^= block[pos];
    }
    chunk->block_list.erase(chunk->block_list.begin()+match);
    chunk->degree--;
}

/*
 * Takes a chunk that has been fully reduced (meaning it has
 *   degree = block_list.size() == 1) and builds a block out
 *   of it.
 */
unsigned char *LT_Decoder::chunk_to_block(Chunk *chunk) {
    if (chunk->block_list.size() != 1)
        return NULL;
    size_t block_len = context->get_descriptor()->chunk_len;
    unsigned char *block = (unsigned char *)malloc(block_len);
    memset(block, 0, block_len);
    memcpy(block, chunk->data, block_len);

    glob_log.log(5, "Converted chunk %u into block %u.  Contents:\n%s\n",
            chunk->id, chunk->block_list[0], block);
    return block;
}

/*
 * Iterate through the list of unfinished chunks, and see if the
 *   incoming chunk (which will be converted into a block) matches
 *   any.  Note that if it does, we will need to iterate them as well.
 */
void LT_Decoder::add_block (Chunk *in_chunk){
    // Ignore if it reduces to a block we've already seen
    if (context->get_block(in_chunk->block_list[0]) != NULL)
        return;
    std::queue< Chunk * > work_queue;
    for (work_queue.push(in_chunk); !work_queue.empty(); work_queue.pop()) {
        // Extract the block from the finished chunk
        Chunk *work_chunk = work_queue.front();
        unsigned block_id = work_chunk->block_list[0];
        unsigned char *block = chunk_to_block(work_chunk);
        context->set_block(block, block_id);

        // Check all remaining chunks, and if they can be reduced
        // to degree 1, remove them from the list and add them
        // to the work queue.
        for (unsigned int i = 0; i < chunk_list.size(); i++) {
            Chunk *chunk = chunk_list[i];
            reduce(chunk, block_id);
            if (chunk->degree == 1) {
                // Ignore if it reduces to a block we've already seen
                if (context->get_block(chunk->block_list[0]) != NULL)
                    continue;
                work_queue.push(chunk);
                chunk->degree = 0;
            }
        }
    }
    clean_chunks();
}

// Remove decoded chunks from our working list
void LT_Decoder::clean_chunks () {
    for (unsigned i = 0; i < chunk_list.size(); i++) {
        if (chunk_list[i]->degree == 0) {
            Chunk *temp = chunk_list[i];
            chunk_list.erase(chunk_list.begin()+i);
            i--;
            delete temp;
        }
    }
}

bool LT_Decoder::is_ready () {
    Message_Descriptor *msg_desc = context->get_descriptor();
    if (msg_desc == NULL) {
        return false;
    } else if (ready) {
        return true;
    }

    std::vector<unsigned> decoded_blocks;
    context->fill_block_list(&decoded_blocks);
    if (decoded_blocks.size() >= msg_desc->total_blocks) {
        ready = true;
        glob_log.log(2, "Decoded lt message: %zu blocks in %d chunks (%.02f%% overhead)\n",
                msg_desc->total_blocks, chunks_seen,
                100.0 * (chunks_seen- msg_desc->total_blocks) / msg_desc->total_blocks);
        return true;
    } else {
        return false;
    }
}
