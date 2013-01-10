#ifndef __TRADITIONAL_ENCODER_H
#define __TRADITIONAL_ENCODER_H

#include "client_server_encoder.h"

class Traditional_Encoder : public Client_Server_Encoder {
    public:
        Traditional_Encoder(Encoder_Context *ctx) : Client_Server_Encoder(ctx) { }
        virtual ~Traditional_Encoder() { }
};

#endif /* __TRADITIONAL_ENCODER_H */
