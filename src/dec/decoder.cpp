#include <stddef.h>

#include "decoder.h"
#include "client_server_decoder.h"
#include "cooperative_decoder.h"
#include "traditional_decoder.h"
#include "lt_decoder.h"

decoder *get_decoder(msg_t algo) {
    switch (algo) {
        case CLIENT_SERVER:
            return new client_server_decoder();
        case COOP:
            return new cooperative_decoder();
        case TRAD:
            return new traditional_decoder();
        case LT:
            return new lt_decoder();
        case RAPTOR:
            return NULL;  // Not yet implemented
        default:
            return NULL;
    }
    return NULL;
}

