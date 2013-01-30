#include <stddef.h>
#include <string.h>
#include <error.h>

#include "decoder.h"
#include "client_server_decoder.h"
#include "cooperative_decoder.h"
#include "lt_decoder.h"
#include "traditional_decoder.h"

#include "logger.h"

Incoming_Message::Incoming_Message(msg_t algo) :
    descriptor(NULL)
{
    decoder = get_decoder(algo);
}

Incoming_Message::~Incoming_Message() {
    // Get rid of all the chunks and blocks
    typedef std::map<unsigned, unsigned char *>::iterator it_type;
    for(it_type it = blocks.begin(); it != blocks.end(); it++) {
        free(it->second);
    }
    for(it_type it = chunks.begin(); it != chunks.end(); it++) {
        free(it->second);
    }

    free(decoder);
    free(descriptor);
}

void Incoming_Message::add_chunk(unsigned char *data, size_t len, int chunk_id) {
    // If the decoder already has the message, we can just throw this out
    if (decoder->is_ready()) {
        return;
    }

    // Deal with the descriptor
    if (chunk_id == -1) {
        if (descriptor != NULL) {
            // Already done, don't worry about it
            return;
        }
        // Copy the descriptor
        // We'll malloc as much as they gave us, in case there's some
        // extra, alg-specific stuff hidden beneath
        descriptor = (Message_Descriptor *) malloc(len);
        memcpy(descriptor, data, len);

        glob_log.log(3, "Recieved descriptor (id -1): total_blocks %zu, chunk_len %zu, num_peers %zu.\n",
                descriptor->total_blocks,
                descriptor->chunk_len,
                descriptor->num_peers);

        return;
    }

    // Don't allocate space for a chunk we've already read
    if (chunks.find(chunk_id) != chunks.end())
        return;

    // Add the chunk to our list
    unsigned char *chunk = (unsigned char *) malloc(len);
    memcpy(chunk, data, len);
    chunks[chunk_id] = chunk;

    decoder->notify(chunk_id);

    // The remaining code is just log stuff.
    glob_log.log(3, "Added chunk %u\n", chunk_id);
    glob_log.log(3, "Current chunks:");

    std::vector<unsigned> chunk_list;
    std::vector<unsigned>::iterator it;
    fill_chunk_list(&chunk_list);
    for (it = chunk_list.begin(); it != chunk_list.end(); it++) {
        glob_log.log(3, " %u", *it);
    }
    glob_log.log(3, "\n");
    glob_log.log(3, "Current blocks:");

    std::vector<unsigned> block_list;
    fill_block_list(&block_list);
    for (it = block_list.begin(); it != block_list.end(); it++) {
        glob_log.log(3, " %u", *it);
    }
    glob_log.log(3, "\n");
}

bool Incoming_Message::is_ready () {
    return decoder->is_ready();
}

bool Incoming_Message::should_forward() {
    return decoder->should_forward();
}

size_t Incoming_Message::get_len() {
    if (!decoder->is_ready())
        return 0;
    return blocks.size() * descriptor->chunk_len;
}

unsigned char *Incoming_Message::get_message() {
    if (!decoder->is_ready()) {
        return NULL;
    }

    size_t data_len = get_len();
    unsigned char *data, *dp;
    data = (unsigned char *) malloc(data_len);
    dp = data;

    for (unsigned b = 0; b < blocks.size(); b++) {
        memcpy(dp, blocks[b], descriptor->chunk_len);
        dp += descriptor->chunk_len;
    }

    return data;
}

// Note: memory for the following two functions is OWNED BY THE CALLER
void Incoming_Message::fill_block_list(std::vector<unsigned> *dest) {
    if(dest == NULL)
        error(-1, 0, "'dest' is NULL!");

    dest->clear();

    typedef std::map<unsigned, unsigned char *>::iterator map_it;
    for (map_it it = blocks.begin(); it != blocks.end(); it++) {
        dest->push_back(it->first);
    }
}

void Incoming_Message::fill_chunk_list(std::vector<unsigned> *dest) {
    if(dest == NULL)
        error(-1, 0, "'dest' is NULL!");

    dest->clear();

    typedef std::map<unsigned, unsigned char *>::iterator map_it;
    for (map_it it = chunks.begin(); it != chunks.end(); it++) {
        dest->push_back(it->first);
    }
}

unsigned char *Incoming_Message::get_block(unsigned index) {
    if (blocks.find(index) == blocks.end())
        return NULL;
    return blocks[index];
}

unsigned char *Incoming_Message::get_chunk(unsigned index) {
    if (chunks.find(index) == chunks.end())
        return NULL;
    return chunks[index];
}

void Incoming_Message::set_block(unsigned char *data, unsigned index) {
    // data has been passed back down to us, so we own it.
    unsigned char *block = get_block(index);
    if (block != NULL) {
        free(block);
    }
    blocks[index] = data;
}

Message_Descriptor *Incoming_Message::get_descriptor() {
    return descriptor;
}

Decoder *Incoming_Message::get_decoder(msg_t algo) {
    switch (algo) {
        case CLIENT_SERVER:
            return new Client_Server_Decoder((Decoder_Context *)this);
        case TRAD:
            return new Traditional_Decoder((Decoder_Context *)this);
        case COOP:
            return new Cooperative_Decoder((Decoder_Context *)this);
        /*
        case LT:
            return new LT_Decoder((Decoder_Context *)this);
            */
        case RAPTOR:
            return NULL;  // Not yet implemented
        default:
            return NULL;
    }
    return NULL;
}

