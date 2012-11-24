#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cooperative_decoder.h"
#include "cooperative_encoder.h"

cooperative_decoder::cooperative_decoder() :
    msg_desc(NULL),
    decoded_data(NULL),
    data_len(0)
{ }

cooperative_decoder::~cooperative_decoder() {
    if (msg_desc) free(msg_desc);
    if (decoded_data) free(decoded_data);
}

void cooperative_decoder::add_chunk (unsigned char * d, size_t len, unsigned int id){
    if (id == 0) {
        if (msg_desc != NULL) {
            // We've already been inited, which means that this is just a
            // transmission terminator.  Ignore it
            return;
        }
        // Read the message descriptor
        msg_desc = (struct coop_descriptor *)malloc(sizeof(struct coop_descriptor));
        memcpy(msg_desc, d, sizeof(coop_descriptor));
        chunk_map[id] = NULL;
    } else {
        unsigned char *data = (unsigned char *) malloc(len);
        memcpy(data, d, len);
        chunk_map[id] = data;
        data_len += len;
    }
}

bool cooperative_decoder::is_done (){
    if (msg_desc->total_chunks == 0) {
        return false;
    } else {
        return chunk_map.size() >= msg_desc->total_chunks;
    }
}

unsigned char * cooperative_decoder::get_message (){
    if (!is_done()) {
        return NULL;
    } else if (decoded_data != NULL) {
        return decoded_data;

    } else {
        // Decode the message
        decoded_data = (unsigned char *) malloc(data_len);
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
}

size_t cooperative_decoder::get_len (){
    if (!is_done()) {
        return 0;
    } else {
        return data_len;
    }
}


