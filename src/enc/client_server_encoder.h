#ifndef __CLIENT_SERVER_ENCODER_H
#define __CLIENT_SERVER_ENCODER_H

#include "encoder.h"

class Client_Server_Encoder : public Encoder {
    public:
        Client_Server_Encoder(Encoder_Context *ctx);
        virtual ~Client_Server_Encoder() { }
        std::vector<unsigned> *get_chunk_list(unsigned peer);
        size_t get_chunk(unsigned char **dest, unsigned chunk_id);
};

#endif /* __CLIENT_SERVER_ENCODER_H */

