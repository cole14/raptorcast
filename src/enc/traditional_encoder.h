#ifndef __TRADITIONAL_ENCODER_H
#define __TRADITIONAL_ENCODER_H

#include "client_server_encoder.h"

class traditional_encoder : public Client_Server_Encoder {
    public:
        traditional_encoder(Encoder_Context *ctx) : Client_Server_Encoder(ctx) { }
        virtual ~traditional_encoder() { }
};

#endif /* __TRADITIONAL_ENCODER_H */
