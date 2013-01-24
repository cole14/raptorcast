#ifndef __COOPERATIVE_DECODER_H
#define __COOPERATIVE_DECODER_H

#include <stddef.h>
#include <map>

#include "decoder.h"

class Cooperative_Decoder : public Client_Server_Decoder {
    public:
        Cooperative_Decoder(Decoder_Context *ctx): Client_Server_Decoder(ctx) {}
        virtual ~Cooperative_Decoder() {}
        bool should_forward () { return true; }
};

#endif /* __COOPERATIVE_DECODER_H */

