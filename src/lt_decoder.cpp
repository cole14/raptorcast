#include <error.h>
#include <errno.h>
#include <functional>
#include <queue>
#include <stdlib.h>
#include <string.h>

#include "lt_decoder.h"
#include "logger.h"

lt_decoder::lt_decoder() :
    msg_desc(NULL)
{ }

lt_decoder::~lt_decoder()
{ }

void lt_decoder::add_chunk (unsigned char * data, size_t len, unsigned int chunk_id) {
    if (len == 0) // End of transmission, ignore
        return;

    if (chunk_id == 0) {
        if (msg_desc != NULL) // Repeat or terminater, ignore it.
            return;

        // Read the message descriptor
        msg_desc = new lt_descriptor();
        memcpy(msg_desc, data, sizeof(lt_descriptor));
        glob_log.log(3, "Read lt message descriptor (chunk 0)!\n");
        glob_log.log(3, "total_chunks %u, num_peers %u, chunk_len %u, seed %d\n",
                msg_desc->total_chunks, msg_desc->num_peers,
                msg_desc->chunk_len, msg_desc->seed);
        return;
    }

    // At this point, it would make sense to check whether we've seen
    //   the message descriptor is always the first chunk in a transmission.
    //   So, we __should__ be good to go.  Check anyway.
    if (msg_desc == NULL)
        error(-1, EIO, "Recieved lt chunk %u before reading header", chunk_id);

    // OK, so now we know that this is neither the message descriptor
    //   nor a transmission terminator, and that we've already
    //   seen the message descriptor.  So, we can process the chunk.
    glob_log.log(3, "Read lt chunk %u!\n", chunk_id);

    Chunk *chunk = new Chunk();
    chunk->id = chunk_id;
    chunk->data = data;
    build_block_list(chunk);
    chunk_list.push_back(chunk);

    for (unsigned int i = 0; i < msg_desc->total_chunks; i++) {
        if (decoded_blocks.find(i) != decoded_blocks.end())   // Fuck C++
            reduce(chunk, decoded_blocks[i]);
    }

    if (chunk->degree == 1) {
        add_block(chunk);
    }
}

/*
 * Iterate the random number generator up to where we figure
 * out what blocks go into the chunk corresponding to this
 * chunk's id
 *
 * XXX This is terrible, we regen the same data over and over again.
 *   It would make much more sense to save generated block lists, seeing
 *   as we'll need many of them eventually.
 */
void lt_decoder::build_block_list(Chunk *chunk) {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> block_count_dist(1, msg_desc->max_degree);
    std::uniform_int_distribution<int> block_select_dist(0, msg_desc->total_blocks);

    generator.seed(msg_desc->seed);
    auto count_rng = std::bind(block_count_dist, generator);
    auto select_rng = std::bind(block_select_dist, generator);

    int num_blocks;
    int *selected_blocks = NULL;
    for (unsigned int cid = 1; cid <= chunk->id; cid++) {
        if (selected_blocks)
            delete[] selected_blocks;
        num_blocks = count_rng();
        selected_blocks = new int[num_blocks];
        for (int i = 0; i < num_blocks; i++) {
            int block_id;
            bool valid;
            do {
                block_id = select_rng();
                valid = true;
                for (int j = 0; j < i; j++) {
                    if (selected_blocks[j] == block_id) {
                        valid = false;
                        break;
                    }
                }
            } while (!valid);
            selected_blocks[i] = block_id;
        }
    }

    glob_log.log(3, "Generated block list for chunk %u: ", chunk->id);
    for (int i = 0; i < num_blocks; i++) {
        chunk->block_list.push_back((unsigned) selected_blocks[i]);
        glob_log.log(3, "%d ", selected_blocks[i]);
    }
    chunk->degree = chunk->block_list.size();
    glob_log.log(3, "\n");

    delete[] selected_blocks;
}

/*
 * checks if a block is in a chunk's block list, and if so,
 * reduces the chunk by the block.
 */
void lt_decoder::reduce (Chunk *chunk, Block *block) {
    int match = -1;
    for (unsigned int i = 0; i < chunk->block_list.size(); i++) {
        if (chunk->block_list[i] == block->id) {
            match = i;
            break;
        }
    }

    if (match < 0) {
        return;
    }

    glob_log.log(3, "Found a match! Reducing chunk %d by block %d\n",
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
lt_decoder::Block *lt_decoder::chunk_to_block(Chunk *chunk) {
    if (chunk->block_list.size() != 1)
        return NULL;
    Block *block = new Block();
    block->data = chunk->data;
    block->id = chunk->block_list[0];
    return block;
}

/*
 * Iterate through the list of unfinished chunks, and see if the
 * incoming chunk (which will be converted into a block) matches
 * any.  Note that if it does, we will need to iterate them as well.
 */
void lt_decoder::add_block (Chunk *in_chunk){
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

void lt_decoder::clean_chunks () {
}

bool lt_decoder::is_ready () {
    return false;
}
bool lt_decoder::is_finished () {
    return false;
}
unsigned char * lt_decoder::get_message () {
    return NULL;
}
size_t lt_decoder::get_len () {
    return 4;
}
