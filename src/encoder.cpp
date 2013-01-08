#include <stddef.h>

#include "encoder.h"
#include "client_server_encoder.h"
#include "cooperative_encoder.h"
#include "traditional_encoder.h"
#include "lt_encoder.h"

Outgoing_Message::~Outgoing_Message() {
    free(padded_data);
}

// Interface for the broadcast channel
Outgoing_Message::Outgoing_Message(msg_t algo, char *d, size_t d_len,
        size_t n_peers, size_t c_len) {
    //Sanity checks
    if(d == NULL)
        error(-1, 0, "Unable to initialize outgoing message with NULL data");
    if(d_len == 0)
        error(-1, 0, "Unable to initialize outgoing message with zero-length data");
    if(c_len == 0)
        error(-1, 0, "Unable to initialize outgoing message "
                "with zero-length chunks");
    if (n_peers == 0)
        error(-1, 0, "Unable to initialize outgoing message with zero peers");

    data_len = dl;
    chunk_len = cl;
    num_peers = np;

    num_blocks = (size_t) ceil(1.0 * data_len / chunk_len);

    // Split up the data into blocks
    split_blocks(d, dl);

    // Pick an encoder type to use
    encoder = get_encoder(algo);
}

std::vector<char *> *Outgoing_Message::get_chunks(unsigned peer) {
    return NULL;
}

// Interface for the encoder
char *Outgoing_Message::get_block(unsigned index) {
    char *ptr = data;
    ptr += index * chunk_len;
    if (ptr > data + data_len) return NULL;
    return ptr;
}

size_t Outgoing_Message::get_num_blocks() {
    return num_blocks;
}

size_t Outgoing_Message::get_block_len() {
    return chunk_len;
}

size_t Outgoing_Message::get_num_peers() {
    return num_peers;
}

/* Generates a vector of pointers to the different blocks in the data */
void Outgoing_Message::split_blocks(unsigned char *data, size_t data_len) {
    // Pad the data to an integer number of chunks
    size_t padded_size = total_chunks * chunk_len;
    padded_data = (unsigned char *) malloc(padded_size);
    memset(padded_data, 0, padded_size);
    memcpy(padded_data, data, data_len);

    // Split up the data
    unsigned char *b_pos;
    for (b_pos = padded_data; b_pos < padded_data+padded_size; b_pos += chunk_len) {
        blocks.push_back(b_pos);
    }
}

Encoder *Outgoing_Message::get_encoder(msg_t algo){
    switch(algo){
        case CLIENT_SERVER:
            return new Client_Server_Encoder();
            /*
        case COOP:
            return new Cooperative_Encoder();
        case TRAD:
            return new Traditional_Encoder();
        case LT:
            return new Lt_Encoder();
            */

        default: return NULL;
    }
}

