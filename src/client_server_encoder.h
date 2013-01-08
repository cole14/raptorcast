#ifndef __CLIENT_SERVER_ENCODER_H
#define __CLIENT_SERVER_ENCODER_H

#include "encoder.h"

class Client_Server_Encoder : public Encoder {
    public:
        Client_Server_Encoder(Encoder_Context *ctx);
        virtual ~Client_Server_Encoder() { }
        std::vector<int> *get_chunk_list(unsigned peer);
        size_t get_chunk(unsigned char **dest, unsigned chunk_id);

    private:
        unsigned char *data;
        size_t data_pos;
        size_t data_len;
        unsigned int next_chunk_id;
        size_t chunk_len;
        size_t num_peers;
};

#endif /* __CLIENT_SERVER_ENCODER_H */

