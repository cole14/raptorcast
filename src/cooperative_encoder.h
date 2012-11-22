#ifndef __COOPERATIVE_ENCODER_H
#define __COOPERATIVE_ENCODER_H

#include "encoder.h"

class cooperative_encoder : public encoder {
    public:
        cooperative_encoder();
        virtual ~cooperative_encoder() { }
        int generate_chunk(unsigned char **dest);
        void init(unsigned char *data, size_t data_len, size_t chunk_len, size_t num_peers);
        void next_stream();

    private:
        unsigned char *data;
        size_t data_pos;
        size_t data_len;
        size_t chunk_len;
        size_t num_peers;
        size_t chunks_per_peer;
        size_t current_peer_chunks;
};

#endif /* __COOPERATIVE_ENCODER_H */
