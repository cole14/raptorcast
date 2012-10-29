//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <error.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client.h"

#include <iostream>

/*
 * Prints the usage statement.
 */
static void usage(void){
    fprintf(stderr, "Usage: %s <name> <port>\n", program_invocation_short_name);
}

/*
 * Constructor. Initializes the broadcast_channel with this client's
 * self-identifying information.
 */
client::client(std::string name, int port)
:chan(NULL), name(name)
{
    chan = new broadcast_channel(name, port, this);
}

/*
 * Destructor. Destroys the broadcast_channel.
 */
client::~client(){
    delete chan;
}

/*
 * This function is called when the broadcast_channel receives a message.
 */
void client::receive(unsigned char *buf, size_t buf_len){
    if(buf == NULL) return;

    fprintf(stdout, "Received message:\n");
    for(size_t i = 0; i < buf_len; i++){
        fprintf(stdout, "%c", buf[i]);
    }
    fprintf(stdout, "\n");
}

/*
 * Reads a hostname from stdin. Trims off the trailing newline.
 */
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

/*
 * Reads a port from stdin. Checks to make sure it's in the valid range for a port.
 */
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
    if(port < 1 || port > 65535){
        error(-1, EINVAL, "Port must be in range [1, 65535]");
    }

    return port;
}

/*
 * Connects to the broadcast group. This asks the user for
 * the hostname and port of the bootstrap node to connect to, and then
 * uses the broadcast channel to make the connection.
 */
void client::connect(){
    int port = 0;

    //Get the bootstrap node's hostname:port from the user
    fprintf(stdout, "Enter bootstrap hostname: ");
    std::string bootstrap_host(read_hostname());

    if (!bootstrap_host.empty()) {
        fprintf(stdout, "Enter bootstrap port: ");
        port = read_port();
    }

    //Connect to the broadcast group through the bootstrap node
    if(!chan->join(bootstrap_host, port)){
        error(-1, 0, "Unable to connect to bootstrap node (%s:%d)!",
	    bootstrap_host.c_str(), port);
    }

    fprintf(stdout, "Successfully connected!\n");
}

void client::run_cli() {
    int max_line = 256;
    char line[max_line];

    while (1) {
        printf("Enter command\n");
        memset(line, 0, sizeof(line));
        while (fgets(line, max_line, stdin) != NULL) {
            // Eliminate trailing whitespace
            for (int i = strlen(line)-1; i >=0; i--) {
                if (isspace(line[i])) line[i] = '\0';
            }

            if (strlen(line) == 0) continue;

            if (strcmp(line, "peers") == 0 || strcmp(line, "p") == 0) {
                printf("Printing list of peers\n");
                chan->print_peers();

            } else if (strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
                printf("Quitting\n");

            } else if (strcmp(line, "help") == 0 || strcmp(line, "h") == 0) {
                printf("Commands: [p]eers, [q]uit, [h]elp\n");

            } else {
                printf("Invalid command: %s\n", line);
            }
        }
    }
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

    // Connect to the group
    c.connect();

    // Run the command line interface
    c.run_cli();

    return 0;
}

