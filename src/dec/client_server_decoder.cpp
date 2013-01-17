#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <error.h>

#include "client_server_decoder.h"

Client_Server_Decoder::Client_Server_Decoder():
    chunks(),
    ready(false)
{ }

Client_Server_Decoder::~Client_Server_Decoder(){
    for(std::list< std::pair< unsigned char *, size_t > >::iterator it = chunks.begin(); it != chunks.end(); it++)
        free(it->first);
}

void Client_Server_Decoder::add_chunk (unsigned char * d, size_t len, unsigned int id) {
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

bool Client_Server_Decoder::is_ready (){
    return ready;
}

bool Client_Server_Decoder::is_finished (){
    return ready;
}

unsigned char * Client_Server_Decoder::get_message (){
    unsigned char *decoded_data = (unsigned char *)malloc(sizeof(unsigned char) * get_len());

    size_t len = 0;
    for(std::list< std::pair< unsigned char *, size_t > >::iterator it = chunks.begin(); it != chunks.end(); it++){
        memcpy(decoded_data + len, it->first, it->second);
        len += it->second;
    }

    return decoded_data;
}

size_t Client_Server_Decoder::get_len (){
    size_t len = 0;
    for(std::list< std::pair< unsigned char *, size_t > >::iterator it = chunks.begin(); it != chunks.end(); it++)
        len += it->second;
    return len;
}


