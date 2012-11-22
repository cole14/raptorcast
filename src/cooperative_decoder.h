#ifndef __COOPERATIVE_DECODER_H
#define __COOPERATIVE_DECODER_H

#include <stddef.h>

#include "decoder.h"

class cooperative_decoder : public decoder {
    public:
        cooperative_decoder();
        virtual ~cooperative_decoder() { }
        void add_chunk (unsigned char * data, size_t len);
        bool is_done ();
        unsigned char * get_message ();
        size_t get_len ();
    private:
        unsigned char *data;
        size_t data_len;
        bool done;
};

#endif /* __COOPERATIVE_DECODER_H */

