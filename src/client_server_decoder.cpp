#include <stdlib.h>
#include <string.h>

#include "client_server_decoder.h"

client_server_Decoder::client_server_Decoder()
{ }

void client_server_Decoder::add_chunk (unsigned char * d, size_t len){
    data = (unsigned char *)realloc(data, data_len + len);
    memcpy(data + data_len, d, len);
}

bool client_server_Decoder::is_done (){
    //TODO: Figure out how to do this...
    return false;
}

unsigned char * client_server_Decoder::get_message (){
    return data;
}

size_t client_server_Decoder::get_len (){
    return data_len;
}


