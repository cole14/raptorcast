#ifndef __COOPERATIVE_ENCODER_H
#define __COOPERATIVE_ENCODER_H

#include "encoder.h"

class Cooperative_Encoder : public Encoder {
    public:
        Cooperative_Encoder(Encoder_Context *ctx);
        virtual ~Cooperative_Encoder() { }
        std::vector<unsigned> *get_chunk_list(unsigned peer);
        size_t get_chunk(unsigned char **dest, unsigned chunk_id);

    private:
        size_t chunks_per_peer;
};

#endif /* __COOPERATIVE_ENCODER_H */
