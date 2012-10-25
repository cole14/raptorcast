//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "client.h"

#include <iostream>

void usage(){
    fprintf(stderr, "Usage: %s <name> <port>\n", program_invocation_short_name);
}

client::client(std::string name, int port)
:chan(NULL), name(name)
{
    chan = new broadcast_channel(name, port, this);
}

client::~client(){
    delete chan;
}

void client::receive(unsigned char *buf, size_t buf_len){
    if(buf == NULL) return;

    fprintf(stdout, "Received message:\n");
    for(size_t i = 0; i < buf_len; i++){
        fprintf(stdout, "%c", buf[i]);
    }
    fprintf(stdout, "\n");
}

void client::run_cli(){
    int port = 0;
    std::string bootstrap_host;

    //Get the bootstrap node's hostname:port from the user
    fprintf(stdout, "Enter bootstrap hostname: ");
    std::cin >> bootstrap_host;
    fprintf(stdout, "Enter bootstrap port: ");
    std::cin >> port;

    //Connect to the broadcast group through the bootstrap node
    if(!chan->connect(bootstrap_host, port)){
        error(-1, 0, "Unable to connect to bootstrap node!");
    }
    fprintf(stdout, "Successfully connected!\n");
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

    //Initialize the client
    client c(name, port);

    //Run the client
    c.run_cli();

    return 0;
}

