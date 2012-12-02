#ifndef __TRADITIONAL_DECODER_H
#define __TRADITIONAL_DECODER_H

#include "client_server_decoder.h"

class traditional_decoder : public client_server_decoder {
    public:
        traditional_decoder() { }
        virtual ~traditional_decoder() { }
        bool should_forward () { return true; }
        bool is_finished() { return false; }
};

#endif /* __TRADITIONAL_DECODER_H */
