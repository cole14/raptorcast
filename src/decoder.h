#ifndef __DECODER_H
#define __DECODER_H

#include "message_types.h"

class decoder {
    public:
        virtual ~decoder() { }
        virtual void add_chunk (unsigned char * data, size_t len, unsigned int chunk_id) = 0;
        virtual bool is_done () = 0;
        virtual unsigned char * get_message () = 0;
        virtual size_t get_len () = 0;
        virtual bool should_forward() = 0;
};

// Construct a decoder of the appropriate type
decoder *get_decoder(msg_t algo);

#endif /* __DECODER_H */
