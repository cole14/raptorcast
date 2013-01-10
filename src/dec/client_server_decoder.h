#ifndef __CLIENT_SERVER_DECODER_H
#define __CLIENT_SERVER_DECODER_H

#include <stddef.h>

#include <list>

#include "decoder.h"

class client_server_decoder : public decoder {
    public:
        client_server_decoder();
        virtual ~client_server_decoder();
        void add_chunk (unsigned char * data, size_t len, unsigned int chunk_id);
        bool is_ready ();
        bool is_finished ();
        unsigned char * get_message ();
        size_t get_len ();
        bool should_forward () { return false; }
    private:
        std::list< std::pair< unsigned char *, size_t > > chunks;
        bool ready;
};

#endif /* __CLIENT_SERVER_DECODER_H */
