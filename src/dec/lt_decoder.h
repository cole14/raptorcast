#ifndef __LT_DECODER_H
#define __LT_DECODER_H

#include <vector>
#include <map>

#include "decoder.h"
#include "lt_common.h"

class LT_Decoder : public Decoder {
    public:
        LT_Decoder(Decoder_Context *ctx);
        ~LT_Decoder();
        void notify (unsigned chunk_id);
        bool is_ready();
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

        LT_Descriptor *msg_desc;
        lt_selector *lts;

        int chunks_seen;
        bool ready;
};

#endif /* __LT_DECODER_H */

