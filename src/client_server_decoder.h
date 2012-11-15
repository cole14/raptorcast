#ifndef __CLIENT_SERVER_DECODER_H
#define __CLIENT_SERVER_DECODER_H

#include <stddef.h>

#include "decoder.h"

class client_server_Decoder : public Decoder {
    public:
        client_server_Decoder();
        virtual ~client_server_Decoder() { }
        void add_chunk (unsigned char * data, size_t len);
        bool is_done ();
        unsigned char * get_message ();
        size_t get_len ();
    private:
        unsigned char *data;
        size_t data_len;
};

#endif /* __CLIENT_SERVER_DECODER_H */

