#ifndef __TRADITIONAL_DECODER_H
#define __TRADITIONAL_DECODER_H

#include "client_server_decoder.h"

class Traditional_Decoder : public Client_Server_Decoder {
    public:
        Traditional_Decoder(Decoder_Context *ctx): Client_Server_Decoder(ctx) { }
        virtual ~Traditional_Decoder() { }
        bool should_forward () { return true; }
        bool is_finished() { return false; }
};

#endif /* __TRADITIONAL_DECODER_H */
