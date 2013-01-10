#ifndef __COOPERATIVE_ENCODER_H
#define __COOPERATIVE_ENCODER_H

#include "encoder.h"

struct coop_descriptor {
    size_t total_chunks;
    size_t num_peers;
    size_t chunk_len;
};

class Cooperative_Encoder : public Encoder {
    public:
        Cooperative_Encoder(Encoder_Context *ctx);
        virtual ~Cooperative_Encoder() { }
        std::vector<unsigned> *get_chunk_list(unsigned peer);
        size_t get_chunk(unsigned char **dest, unsigned chunk_id);

    private:
        unsigned char *data;
        size_t data_pos;
        size_t data_len;
        size_t chunk_len;
        size_t num_peers;
        size_t total_chunks;
        size_t chunks_per_peer;
        size_t current_peer_chunks;

        bool descriptor_sent;
        unsigned int chunk_id;
};

#endif /* __COOPERATIVE_ENCODER_H */
