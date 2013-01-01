#include <stddef.h>

#include "encoder.h"
#include "client_server_encoder.h"
#include "cooperative_encoder.h"
#include "traditional_encoder.h"
#include "lt_encoder.h"

// Interface for the broadcast channel
Outgoing_Message::Outgoing_Message(char *d, size_t d_len,
        size_t n_peers, size_t c_len) {
}

std::vector<char *> *Outgoing_Message::get_chunks(unsigned peer) {
    return NULL;
}

// Interface for the encoder
char *Outgoing_Message::get_block(unsigned index) {
    return NULL;
}

size_t Outgoing_Message::get_num_blocks() {
    return ceil(data_len / chunk_len);
}

size_t Outgoing_Message::get_block_len() {
    return chunk_len;
}

size_t Outgoing_Message::get_num_peers() {
    return num_peers;
}

encoder *get_encoder(msg_t algo){
    switch(algo){
        case CLIENT_SERVER:
            return new client_server_encoder();
        case COOP:
            return new cooperative_encoder();
        case TRAD:
            return new traditional_encoder();
        case LT:
            return new lt_encoder();

        default: return NULL;
    }
}

