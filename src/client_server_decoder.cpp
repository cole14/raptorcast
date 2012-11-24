#include <stdlib.h>
#include <string.h>

#include "client_server_decoder.h"

client_server_decoder::client_server_decoder()
:data(NULL),
data_len(0),
done(false)
{ }

void client_server_decoder::add_chunk (unsigned char * d, size_t len, unsigned int id) {
    if (len == 0) {
        done = true;
        return;
    }
    data = (unsigned char *)realloc(data, data_len + len);
    memcpy(data + data_len, d, len);
    data_len += len;
}

bool client_server_decoder::is_done (){
    return done;
}

unsigned char * client_server_decoder::get_message (){
    return data;
}

size_t client_server_decoder::get_len (){
    return data_len;
}


