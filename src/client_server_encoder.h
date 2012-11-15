#ifndef __CLIENT_SERVER_ENCODER_H
#define __CLIENT_SERVER_ENCODER_H

#include "encoder.h"

class client_server_Encoder : public Encoder {
    public:
        client_server_Encoder();
        virtual ~client_server_Encoder() { }
        unsigned char * generate_chunk();
        void init(unsigned char *data, size_t data_len, size_t chunk_len);

    private:
        unsigned char *data;
        size_t data_pos;
        size_t data_len;
        size_t chunk_len;
};

#endif /* __CLIENT_SERVER_ENCODER_H */

