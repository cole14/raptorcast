
#include "iter_tcp_channel.h"

//Default constructor
iter_tcp_channel::iter_tcp_channel(int port)
:broadcast_channel(port)
{
}

//Default destructor
iter_tcp_channel::~iter_tcp_channel(void){
}

void iter_tcp_channel::connect(std::string& ip_port){
}

void iter_tcp_channel::send(unsigned char *buf, size_t buf_len){
}

size_t iter_tcp_channel::recv(unsigned char *buf, size_t buf_len){
    return 0;
}

