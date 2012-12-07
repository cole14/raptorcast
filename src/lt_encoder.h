#ifndef __LT_ENCODER_H
#define __LT_ENCODER_H

#include <vector>
#include <random>

#include "encoder.h"
#include "logger.h"
#include "lt_common.h"



class lt_encoder : public encoder {
    public :
        lt_encoder();
        virtual ~lt_encoder() {}
        void init(unsigned char *data, size_t data_len, size_t chunk_len, size_t num_peers);
        int generate_chunk(unsigned char **dest, unsigned int *chunk_id);
        void next_stream();

        void split_blocks(unsigned char *data, size_t data_len);

        struct lt_descriptor *get_desc();

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

        lt_selector *lts;
};

#endif // __LT_ENCODER_H
