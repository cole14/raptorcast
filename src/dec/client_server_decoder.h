#ifndef __CLIENT_SERVER_DECODER_H
#define __CLIENT_SERVER_DECODER_H

#include <stddef.h>

#include <list>

#include "decoder.h"

class Client_Server_Decoder : public Decoder {
    public:
        Client_Server_Decoder(Decoder_Context *ctx) : Decoder(ctx) {}
        ~Client_Server_Decoder() {}
        void notify (unsigned chunk_id);
        bool is_ready ();
        bool should_forward () { return false; }
};

#endif /* __CLIENT_SERVER_DECODER_H */

