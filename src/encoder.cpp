#include <stddef.h>

#include "encoder.h"
#include "client_server_encoder.h"
#include "cooperative_encoder.h"

encoder *get_encoder(msg_t algo){
    switch(algo){
        case CLIENT_SERVER:
            return new client_server_encoder();
        case COOP:
            return new cooperative_encoder();

        default: return NULL;
    }
}

