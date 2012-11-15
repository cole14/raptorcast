#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <error.h>

#include "client_server_encoder.h"

client_server_Encoder::client_server_Encoder()
:data(NULL),data_pos(0),data_len(0),chunk_len(0)
{ }

void client_server_Encoder::init(unsigned char *d, size_t dl, size_t cl){
    //Sanity checks
    if(d == NULL)
        error(-1, 0, "Unable to initialize client_server_Encoder with NULL data");

    data = d;
    data_len = dl;
    chunk_len = cl;

    if(data_len == 0)
        error(-1, 0, "Unable to initialize client_server_Encoder with zero-length data");
    if(chunk_len == 0)
        error(-1, 0, "Unable to initialize client_server_Encoder with zero-length chunks");
}

unsigned char * client_server_Encoder::generate_chunk(){
    unsigned char *chunk = NULL;
    size_t len = data_len - data_pos;
    if(len > chunk_len) len = chunk_len;

    if(data_len == 0) return NULL;

    chunk = (unsigned char *)calloc(len, sizeof(unsigned char));
    memcpy(chunk, data + data_pos, len);
    data_pos += len;

    return chunk;
}
