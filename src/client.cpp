//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "iter_tcp_channel.h"

void usage(){
    fprintf(stderr, "Usage: %s <port>\n", program_invocation_short_name);
}

int main(int argc, char *argv[]){
    if(argc != 2){
        usage();
        exit(EINVAL);
    }

    //Parse the port number
    int port = (int)strtol(argv[1], NULL, 10);

    //Create and destroy an iterative tcp broadcast channel on port 44123
    iter_tcp_channel *chan = new iter_tcp_channel(port);
    delete chan;

    return 0;
}

