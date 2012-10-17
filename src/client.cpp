//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"

void usage(){
    fprintf(stderr, "Usage: %s <name> <port>\n", program_invocation_short_name);
}

void client::receive(unsigned char *buf, size_t buf_len){
}

client::client(std::string name, int port)
:chan(NULL), name(name)
{
    chan = new broadcast_channel(name, port, this);
}

client::~client(){
    delete chan;
}

int main(int argc, char *argv[]){
    if(argc != 3){
        usage();
        exit(EINVAL);
    }

    //Parse the client name
    std::string name(argv[1]);
    //Parse the port number
    int port = (int)strtol(argv[2], NULL, 10);

    client c(name, port);

    return 0;
}

