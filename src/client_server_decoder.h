#ifndef __CLIENT_SERVER_DECODER_H
#define __CLIENT_SERVER_DECODER_H

#include <stddef.h>

#include "decoder.h"

class client_server_decoder : public decoder {
    public:
        client_server_decoder();
        virtual ~client_server_decoder() { }
        void add_chunk (unsigned char * data, size_t len, unsigned int chunk_id);
        bool is_done ();
        unsigned char * get_message ();
        size_t get_len ();
    private:
        unsigned char *data;
        size_t data_len;
        bool done;
};

#endif /* __CLIENT_SERVER_DECODER_H */

