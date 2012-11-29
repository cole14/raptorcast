#include <stddef.h>

#include "decoder.h"
#include "client_server_decoder.h"
#include "cooperative_decoder.h"

decoder *get_decoder(msg_t algo) {
    switch (algo) {
        case CLIENT_SERVER:
            return new client_server_decoder();
        case COOP:
            return new cooperative_decoder();
        case TRAD:
        case RAPTOR:
            return NULL;  // Not yet implemented
        default:
            return NULL;
    }
    return NULL;
}

