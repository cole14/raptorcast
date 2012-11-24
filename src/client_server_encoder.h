#ifndef __CLIENT_SERVER_ENCODER_H
#define __CLIENT_SERVER_ENCODER_H

#include "encoder.h"

class client_server_encoder : public encoder {
    public:
        client_server_encoder();
        virtual ~client_server_encoder() { }
        int generate_chunk(unsigned char **dest, unsigned int *chunk_id);
        void init(unsigned char *data, size_t data_len, size_t chunk_len, size_t num_peers);
        void next_stream();

    private:
        unsigned char *data;
        size_t data_pos;
        size_t data_len;
        size_t chunk_len;
        size_t num_peers;
};

#endif /* __CLIENT_SERVER_ENCODER_H */

