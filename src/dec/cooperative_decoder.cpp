#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cooperative_decoder.h"
#include "enc/cooperative_encoder.h"

#include "logger.h"

cooperative_decoder::cooperative_decoder() :
    msg_desc(NULL),
    data_len(0)
{ }

cooperative_decoder::~cooperative_decoder() {
    if (msg_desc) free(msg_desc);
}

void cooperative_decoder::add_chunk (unsigned char * d, size_t len, unsigned int id){
    if (len == 0) {
        // End of transmission, ignore
        return;
    } else if (id == 0) {
        if (msg_desc != NULL) {
            // We've already been inited, which means that this is just a
            // transmission terminator.  Ignore it
            return;
        }

        // Read the message descriptor
        glob_log.log(3, "Read message descriptor (chunk 0)!\n");
        msg_desc = (struct coop_descriptor *)malloc(sizeof(struct coop_descriptor));
        memcpy(msg_desc, d, sizeof(coop_descriptor));
        chunk_map[0] = NULL;

    } else if (chunk_map.count(id) > 0) {
        // We've seen this key before.
        return;

    } else {
        glob_log.log(3, "Read chunk %u\n", id);
        unsigned char *data = (unsigned char *) malloc(len);
        memcpy(data, d, len);
        chunk_map[id] = data;
        data_len += len;
    }

    glob_log.log(3, "Current chunks: ");
    std::map<unsigned int, unsigned char *>::iterator it = chunk_map.begin();
    for (; it != chunk_map.end(); ++it) {
        glob_log.log(3, "%u, ", it->first);
    }
    glob_log.log(3, "\n");
}

bool cooperative_decoder::is_ready (){
    if (msg_desc == NULL || msg_desc->total_chunks == 0) {
        return false;
    } else {
        return chunk_map.size() >= msg_desc->total_chunks;
    }
}

bool cooperative_decoder::is_finished (){
    return is_ready();
}

unsigned char * cooperative_decoder::get_message (){
    if (!is_ready()) {
        return NULL;
    }

    // Decode the message
    unsigned char *decoded_data = (unsigned char *) malloc(data_len);
    memset(decoded_data, 0, data_len);
    unsigned char *pos = decoded_data;
    size_t bytes_read = 0;
    // Note that the 0th chunk is the message descriptor, and the last may
    // not be full length
    unsigned int i;
    for (i = 1; i < msg_desc->total_chunks-1; i++) {
        memcpy(pos, chunk_map[i], msg_desc->chunk_len);
        pos += msg_desc->chunk_len;
        bytes_read += msg_desc->chunk_len;
    }
    memcpy(pos, chunk_map[i], data_len-bytes_read);

    return decoded_data;
}

size_t cooperative_decoder::get_len (){
    if (!is_ready()) {
        return 0;
    } else {
        return data_len;
    }
}


