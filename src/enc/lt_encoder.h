#ifndef __LT_ENCODER_H
#define __LT_ENCODER_H

#include <vector>
#include <random>

#include "encoder.h"
#include "logger.h"
#include "lt_common.h"



class LT_Encoder : public Encoder {
    public :
        LT_Encoder(Encoder_Context *ctx);
        virtual ~LT_Encoder() {}
        std::vector<unsigned> *get_chunk_list(unsigned peer);
        size_t get_chunk(unsigned char **dest, unsigned chunk_id);
        Message_Descriptor *get_descriptor();

    private :
        size_t chunks_per_peer;

        lt_selector *lts;
};

#endif // __LT_ENCODER_H
