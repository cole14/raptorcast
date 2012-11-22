#include <stdlib.h>
#include <string.h>

#include "cooperative_decoder.h"

// XXX XXX XXX This is just a copy of CS_decoder!

cooperative_decoder::cooperative_decoder()
:data(NULL),
data_len(0),
done(false)
{ }

void cooperative_decoder::add_chunk (unsigned char * d, size_t len){
    if (len == 0) {
        done = true;
        return;
    }
    data = (unsigned char *)realloc(data, data_len + len);
    memcpy(data + data_len, d, len);
    data_len += len;
}

bool cooperative_decoder::is_done (){
    return done;
}

unsigned char * cooperative_decoder::get_message (){
    return data;
}

size_t cooperative_decoder::get_len (){
    return data_len;
}


