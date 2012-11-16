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
#include <sys/stat.h>

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
client::client(std::string name, std::string port)
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
    if(port < 1 || port > 65534){
        error(-1, EINVAL, "Port must be in range [1, 65534]");
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

msg_t client::get_alg () {
    int max_line = 256;
    char line[max_line];

    while (0xFULL) {
        printf("Which algorithm?\n");
        printf("client-[s]erver, [t]raditional, [c]ooperatvie, [r]aptor, [b]ack\n");

        //Print the prompt
        do {
            printf("%s> ", name.c_str());
            memset(line, 0, sizeof(line));

            //Read the command
            if(fgets(line, max_line, stdin) == NULL) {
                error(-1, errno, "Error reading user input");
            }

            // Eliminate trailing whitespace
            for (int i = strlen(line)-1; i >=0; i--) {
                if (isspace(line[i])) line[i] = '\0';
            }
        } while (strlen(line) == 0);

        //handle the command
        if (strcmp(line, "client-server") == 0 || strcmp(line, "s") == 0) {
            return CLIENT_SERVER;

        } else if (strcmp(line, "traditional") == 0 || strcmp(line, "t") == 0) {
            return TRAD;

        } else if (strcmp(line, "cooperative") == 0 || strcmp(line, "c") == 0) {
            return COOP;

        } else if (strcmp(line, "raptor") == 0 || strcmp(line, "r") == 0) {
            return RAPTOR;

        } else if (strcmp(line, "back") == 0 || strcmp(line, "b") == 0) {
            return QUIT;

        } else {
            printf("Couldn't interpret response: %s\n", line);
            continue;
        }
    }
}


void client::run_cli() {
    int max_line = 256;
    char line[max_line];
    msg_t algorithm;

    while (0xFULL) {
        do {
            //Print the prompt
            printf("%s> ", name.c_str());
            memset(line, 0, sizeof(line));

            //Read the command
            if(fgets(line, max_line, stdin) == NULL) {
                error(-1, errno, "Error reading user input");
            }

            // Eliminate trailing whitespace
            for (int i = strlen(line)-1; i >=0; i--) {
                if (isspace(line[i])) line[i] = '\0';
            }

        } while (strlen(line) == 0);

        //handle the command
        if (strcmp(line, "peers") == 0 || strcmp(line, "p") == 0) {
            printf("Known Peers:\n");
            chan->print_peers(1);

        } else if (strcmp(line, "quit") == 0 || strcmp(line, "q") == 0) {
            printf("Quitting\n");
            chan->quit();
            break;

        } else if (strcmp(line, "help") == 0 || strcmp(line, "h") == 0) {
            printf("Commands: send [t]ext, send [f]ile, [p]eers, [q]uit, [h]elp\n");

        } else if (strcmp(line, "send text") == 0 || strcmp(line, "t") == 0) {
            algorithm = get_alg();
            if (algorithm == QUIT) continue;
            printf("Sending text...\n");

        } else if (strcmp(line, "send file") == 0 || strcmp(line, "f") == 0) {
            //Get the desired broadcast algorithm
            algorithm = get_alg();
            if (algorithm == QUIT) continue;

            //Get the filename
            printf("Input Filename: ");
            if(fgets(line, max_line, stdin) == NULL) {
                error(-1, errno, "Error reading user input");
            }
            while(isspace(line[strlen(line)-1]))
                line[strlen(line)-1] = '\0';

            //Open the file
            FILE *fp = fopen(line, "r");
            if(fp == NULL){
                error(-1, errno, "Error opening '%s'", line);
            }

            //Get the file length
            struct stat fp_stat;
            if(-1 == fstat(fileno(fp), &fp_stat)){
                error(-1, errno, "Error stating '%s'", line);
            }

            //Read the file into an in-memory buffer
            size_t siz = (size_t)fp_stat.st_size;
            unsigned char *file_contents = (unsigned char *)malloc(siz * sizeof(unsigned char));
            if(siz != fread(file_contents, sizeof(unsigned char), siz, fp)){
                error(-1, errno, "Error reading file");
            }

            //Broadcast the file
            printf("Sending file...\n");
            chan->broadcast(algorithm, file_contents, (size_t)siz);

        } else {
            printf("Invalid command: %s\n", line);
        }
    }
}


int main(int argc, char *argv[]){
    if(argc != 3){
        usage();
        exit(EINVAL);
    }

    //Initialize the client
    client c(argv[1], argv[2]);

    // Connect to the group
    c.connect();

    // Run the command line interface
    c.run_cli();

    return 0;
}

