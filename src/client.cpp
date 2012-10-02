//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"
#include "iter_tcp_channel.h"

void usage(){
    fprintf(stderr, "Usage: %s <port>\n", program_invocation_short_name);
}

void client::receive(unsigned char *buf, size_t buf_len){
}

client::client(int port)
:chan(NULL)
{
    chan = new iter_tcp_channel(port, this);
}

client::~client(){
    delete chan;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        usage();
        exit(EINVAL);
    }

    //Parse the port number
    int port = (int)strtol(argv[1], NULL, 10);

    client c(port);

    return 0;
}

