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
        struct Chunk {
            unsigned int id;
            unsigned int degree;
            std::vector<unsigned int> block_list;
            unsigned char *data;
        };

        void build_block_list(Chunk *chunk);
        unsigned char *chunk_to_block(Chunk *chunk);

        void reduce (Chunk *chunk, unsigned block_id);
        void add_block (Chunk *chunk);
        void clean_chunks ();

        // Working set of chunks that haven't been decoded.
        //   Chunks reference memory owned by the decoder context.
        std::vector< Chunk * > chunk_list;

        lt_selector *lts;

        int chunks_seen;
        bool ready;
};

#endif /* __LT_DECODER_H */

