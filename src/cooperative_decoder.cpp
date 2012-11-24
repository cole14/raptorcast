#include <stdlib.h>
#include <string.h>

#include "cooperative_decoder.h"
#include "cooperative_encoder.h"

cooperative_decoder::cooperative_decoder() :
    msg_desc(NULL);
    decoded_data(NULL),
    data_len(0)
{ }

cooperative_decoder::~cooperative_decoder() {
    if (msg_desc) free(msg_desc);
    if (decoded_data) free(decoded_data);
}

#define INSERT(k,v) chunk_map.insert(pair<unsigned int, unsigned char *>(k, v))
void cooperative_decoder::add_chunk (unsigned char * d, size_t len, unsigned int id){
    if (id == 0) {
        // Read the message descriptor
        msg_desc = (struct coop_descriptor *)malloc(sizeof(struct coop_descriptor));
        memcpy(msg_desc, d, sizeof(coop_descriptor));
        INSERT(id, NULL);
    } else {
        unsigned char *data = (unsigned char *) malloc(len);
        memcpy(data, d, len);
        INSERT(id, data);
        data_len += len;
    }
}
#undef INSERT

bool cooperative_decoder::is_done (){
    if (total_chunks == 0) {
        return false;
    } else {
        return map.size() >= total_chunks;
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
        for (int i = 1; i < total_chunks-1; i++) {
            memcpy(pos, chunk_map[i], msg_desc->chunk_len);
            pos += msg_desc->chunk_len;
            bytes_read += msg_desc->chunk_len;
        }
        memcpy(pos, chunk_map[i], data_len-bytes_read);

        return decoded_data;
        // XXX NOT YET TESTED!!!
    }
}

size_t cooperative_decoder::get_len (){
    return data_len;
}


