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

static std::string read_hostname(void){
    size_t n = BUFSIZ;
    char lne[n];
    ssize_t len = 0;

    //Read the line
    if(NULL == fgets(lne, n, stdin)){
        error(-1, errno, "Unable to read hostname");
    }

    //Get rid of the newline terminator
    len = strlen(lne) - 1;
    while(lne[len] == '\r' || lne[len] == '\n')
        lne[len--] = '\0';

    return std::string(lne);
}

static int read_port(void){
    long port = 0;
    size_t n = 128;
    char lne[n];

    //Read the line
    if(NULL == fgets(lne, n, stdin)){
        error(-1, errno, "Unable to read hostname");
    }

    //parse the port
    port = strtol(lne, NULL, 10);
    if(port <= INT_MIN || port >= INT_MAX){
        error(-1, ERANGE, "Unable to read port");
    }
    if(port == 0){
        error(-1, EINVAL, "Port must be in range [1, 65535]");
    }

    return port;
}

void client::run_cli(){
    int port = 0;

    //Get the bootstrap node's hostname:port from the user
    fprintf(stdout, "Enter bootstrap hostname: ");
    std::string bootstrap_host(read_hostname());

    fprintf(stdout, "Enter bootstrap port: ");
    port = read_port();

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

