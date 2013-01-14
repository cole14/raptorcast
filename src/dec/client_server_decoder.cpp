#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>

#include "client_server_decoder.h"

client_server_decoder::client_server_decoder():
    chunks(),
    ready(false)
{ }

client_server_decoder::~client_server_decoder(){
    for(std::list< std::pair< unsigned char *, size_t > >::iterator it = chunks.begin(); it != chunks.end(); it++)
        free(it->first);
}

void client_server_decoder::add_chunk (unsigned char * d, size_t len, unsigned int id) {
    if (ready)
        return;

    if (len == 0) {
        ready = true;
        return;
    }

    unsigned char *chunk_data = (unsigned char *)malloc(sizeof(unsigned char) * len);
    memcpy(chunk_data, d, len);
    chunks.push_back(std::make_pair(chunk_data, len));
}

bool client_server_decoder::is_ready (){
    return ready;
}

bool client_server_decoder::is_finished (){
    return ready;
}

unsigned char * client_server_decoder::get_message (){
    unsigned char *decoded_data = (unsigned char *)malloc(sizeof(unsigned char) * get_len());

    size_t len = 0;
    for(std::list< std::pair< unsigned char *, size_t > >::iterator it = chunks.begin(); it != chunks.end(); it++){
        memcpy(decoded_data + len, it->first, it->second);
        len += it->second;
    }

    return decoded_data;
}

size_t client_server_decoder::get_len (){
    size_t len = 0;
    for(std::list< std::pair< unsigned char *, size_t > >::iterator it = chunks.begin(); it != chunks.end(); it++)
        len += it->second;
    return len;
}


