#ifndef __DECODER_H
#define __DECODER_H

#include <stddef.h>

class decoder {
    public:
        virtual ~decoder() { }
        virtual void add_chunk (unsigned char * data, size_t len, unsigned int chunk_id) = 0;
        virtual bool is_ready () = 0;
        virtual bool is_finished () = 0;
        virtual unsigned char * get_message () = 0;
        virtual size_t get_len () = 0;
        virtual bool should_forward() = 0;
};

#endif /* __DECODER_H */
