#ifndef __LT_ENCODER_H
#define __LT_ENCODER_H

#include <vector>

#include "encoder.h"

class lt_encoder : public encoder {
    public :
        lt_encoder();
        virtual ~lt_encoder() {}
        int generate_chunk(unsigned char **dest, unsigned int *chunk_id);
        void init(unsigned char *data, size_t data_len, size_t chunk_len, size_t num_peers);
        void next_stream();

    private :
        size_t data_len;
        size_t chunk_len;
        size_t num_peers;
        size_t total_chunks;
        size_t chunks_per_peer;
        size_t current_peer_chunks;

        bool descriptor_sent;
        unsigned int chunk_id;

        std::vector<unsigned char *> blocks;    // Data split into blocks
        size_t degree;      // Max number of blocks to choose
        unsigned int seed;  // Seed for the random number generator

        void split_blocks(unsigned char *data, size_t data_len);
};

#endif // __LT_ENCODER_H
