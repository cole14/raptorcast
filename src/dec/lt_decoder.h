#ifndef __LT_DECODER_H
#define __LT_DECODER_H

#include <vector>
#include <map>

#include "decoder.h"
#include "lt_common.h"

class lt_decoder : public decoder {
    public:
        lt_decoder();
        ~lt_decoder();
        void add_chunk (unsigned char * data, size_t len, unsigned int chunk_id);
        bool is_ready ();
        bool is_finished ();
        unsigned char * get_message ();
        size_t get_len ();
        bool should_forward () { return true; }

    private:
        struct Block {
            unsigned int id;
            unsigned char *data;
        };

        struct Chunk {
            unsigned int id;
            unsigned int degree;
            std::vector<unsigned int> block_list;
            unsigned char *data;
        };

        void build_block_list(Chunk *chunk);
        Block *chunk_to_block(Chunk *chunk);

        void reduce (Chunk *chunk, Block *block);
        void add_block (Chunk *chunk);
        void clean_chunks ();

        std::map< unsigned int, Block * > decoded_blocks;
        std::vector< Chunk * > chunk_list;

        lt_descriptor *msg_desc;
        lt_selector *lts;

        int chunks_seen;
        bool ready;
};

#endif /* __LT_DECODER_H */

