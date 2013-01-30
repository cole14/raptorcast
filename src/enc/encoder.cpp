#include <error.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "encoder.h"
#include "client_server_encoder.h"
#include "cooperative_encoder.h"
#include "traditional_encoder.h"
#include "lt_encoder.h"

#include "logger.h"

using namespace std;

Outgoing_Message::~Outgoing_Message() {
    free(padded_data);
    delete encoder;
    // blocks is just a list of pointers, don't need to delete.
}

// Interface for the broadcast channel
Outgoing_Message::Outgoing_Message(msg_t algo, unsigned char *d, size_t d_len,
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

    data_len = d_len;
    chunk_len = c_len;
    num_peers = n_peers;

    num_blocks = (size_t) ceil(1.0 * data_len / chunk_len);

    // Split up the data into blocks
    split_blocks(d, data_len);

    // Pick an encoder type to use
    encoder = get_encoder(algo);
}

vector<pair<int, unsigned char *> > *Outgoing_Message::get_chunks(unsigned peer) {
    // We'll be putting together a list of (id, chunk) pairs
    vector<pair<int, unsigned char *>> *chunks;
    chunks = new vector<pair<int, unsigned char *>>();

    // Add the message descriptor first
    unsigned char *desc = (unsigned char *)encoder->get_descriptor();
    chunks->push_back(make_pair(-1, desc));

    // Ask the encoder which chunks to send to this peer
    vector<unsigned> *chunk_list = encoder->get_chunk_list(peer);

    // Get those chunks and add them
    for (unsigned i = 0; i < chunk_list->size(); i++) {
        unsigned char *chunk;
        encoder->get_chunk(&chunk, (*chunk_list)[i]);
        chunks->push_back(make_pair((*chunk_list)[i], chunk));
    }

    delete chunk_list;
    return chunks;
}

// Interface for the encoder
unsigned char *Outgoing_Message::get_block(unsigned index) {
    if (index >= blocks.size()) return NULL;
    return blocks[index];
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
    size_t padded_size = num_blocks * chunk_len;
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
            return new Client_Server_Encoder((Encoder_Context *)this);
        case TRAD:
            return new Traditional_Encoder((Encoder_Context *)this);
        case COOP:
            return new Cooperative_Encoder((Encoder_Context *)this);
        case LT:
            return new LT_Encoder((Encoder_Context *)this);

        default: return NULL;
    }
}

// Returns a generic message descriptor
Message_Descriptor *Encoder::get_descriptor() {
    Message_Descriptor *desc;
    desc = (Message_Descriptor *)malloc(sizeof(Message_Descriptor));
    desc->total_blocks = context->get_num_blocks();
    desc->chunk_len = context->get_block_len();
    desc->num_peers = context->get_num_peers();
    return desc;
}
