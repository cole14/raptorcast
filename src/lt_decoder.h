#ifndef __LT_DECODER_H
#define __LT_DECODER_H

#include <vector>
#include <map>

#include "decoder.h"
#include "lt_decoder.h"
#include "lt_encoder.h"

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

        Block *chunk_to_block(Chunk *chunk);

        void build_block_list(Chunk *chunk);
        void reduce (Chunk *chunk, Block *block);
        void add_block (Chunk *chunk);
        void clean_chunks ();

        std::map< unsigned int, Block * > decoded_blocks;
        std::vector< Chunk * > chunk_list;

        struct lt_descriptor *msg_desc;
};

#endif /* __LT_DECODER_H */

