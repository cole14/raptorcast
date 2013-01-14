#ifndef __COOPERATIVE_DECODER_H
#define __COOPERATIVE_DECODER_H

#include <stddef.h>
#include <map>

#include "decoder.h"

class cooperative_decoder : public decoder {
    public:
        cooperative_decoder();
        ~cooperative_decoder();
        void add_chunk (unsigned char * data, size_t len, unsigned int chunk_id);
        bool is_ready ();
        bool is_finished ();
        unsigned char * get_message ();
        size_t get_len ();
        bool should_forward () { return true; }
    private:
        std::map<unsigned int, unsigned char *> chunk_map;
        struct coop_descriptor *msg_desc;
        size_t data_len;
};

#endif /* __COOPERATIVE_DECODER_H */

