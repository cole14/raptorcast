//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <error.h>
#include <errno.h>
#include <iostream>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "client.h"

#include "logger.h"


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
Client::Client(std::string name, std::string port)
:line_buf(NULL), line_buf_len(0), line_buf_filled_len(0), chan(NULL), name(name)
{
    chan = new Broadcast_Channel(name, port, this);
}

/*
 * Destructor. Destroys the broadcast_channel.
 */
Client::~Client(){
    free(line_buf);
    delete chan;
}

void Client::print_prompt() {
    printf("\e[0m%s> ", name.c_str());
}

/*
 * This function is called when the broadcast_channel receives a message.
 */
void Client::receive(unsigned char *buf, size_t buf_len){
    if(buf == NULL) return;

    glob_log.log(1, "Received message of len %zu:\n",buf_len);
    for(size_t i = 0; i < buf_len; i++){
        glob_log.log(1, "%c", buf[i]);
    }
    glob_log.log(1, "\n");
    free(buf);
    print_prompt();
    fflush(stdout);
}

char *Client::read_stripped_line(){
    //Read the line
    if(-1 == (line_buf_filled_len = getline(&line_buf, &line_buf_len, stdin))){
        error(-1, errno, "Unable to read hostname");
    }

    //Get rid of the newline terminator
    //Hello, World!\0\0
    while(line_buf_filled_len > 0 && isspace(line_buf[line_buf_filled_len - 1]))
        line_buf[--line_buf_filled_len] = '\0';

    return line_buf;
}

/*
 * Reads a hostname from stdin. Trims off the trailing newline.
 */
std::string Client::read_hostname(void){
    //Construct the return string
    return std::string(read_stripped_line());
}

/*
 * Reads a port from stdin. Checks to make sure it's in the valid range for a port.
 */
int Client::read_port(void){
    long port = 0;

    //Read the line
    read_stripped_line();

    //parse the port
    port = strtol(line_buf, NULL, 10);
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
void Client::connect(){
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

msg_t Client::get_alg () {
    while (0xFULL) {
        printf("Which algorithm?\n");
        printf("client-[s]erver, [t]raditional, [c]ooperative, [l]t, [r]aptor, [b]ack\n");

        do {
            print_prompt();

            //Read the command
            read_stripped_line();
        } while (strlen(line_buf) == 0);

        //handle the command
        if (strcmp(line_buf, "client-server") == 0 || strcmp(line_buf, "s") == 0) {
            return CLIENT_SERVER;

        } else if (strcmp(line_buf, "traditional") == 0 || strcmp(line_buf, "t") == 0) {
            return TRAD;

        } else if (strcmp(line_buf, "cooperative") == 0 || strcmp(line_buf, "c") == 0) {
            return COOP;

        } else if (strcmp(line_buf, "lt") == 0 || strcmp(line_buf, "l") == 0) {
            return LT;

        } else if (strcmp(line_buf, "raptor") == 0 || strcmp(line_buf, "r") == 0) {
            return RAPTOR;

        } else if (strcmp(line_buf, "back") == 0 || strcmp(line_buf, "b") == 0) {
            return QUIT;

        } else {
            printf("Couldn't interpret response: %s\n", line_buf);
            continue;
        }
    }
}


void Client::run_cli() {
    msg_t algorithm;

    while (0xFULL) {
        do {
            print_prompt();

            //Read the command
            read_stripped_line();

        } while (strlen(line_buf) == 0);

        //handle the command
        if (strcmp(line_buf, "peers") == 0 || strcmp(line_buf, "p") == 0) {
            printf("Known Peers:\n");
            chan->print_peers(1);

        } else if (strcmp(line_buf, "quit") == 0 || strcmp(line_buf, "q") == 0) {
            printf("Quitting\n");
            chan->quit();
            break;

        } else if (strcmp(line_buf, "help") == 0 || strcmp(line_buf, "h") == 0) {
            printf("Commands: send [t]ext, send [f]ile, [p]eers, [m]essage-history, [d]ebug-toggle, [l]log-level, [s]pin, [z]ombify, [q]uit, [h]elp\n");

        } else if (strcmp(line_buf, "send text") == 0 || strcmp(line_buf, "t") == 0) {
            algorithm = get_alg();
            if (algorithm == QUIT) continue;

            printf("Input text: ");
            read_stripped_line();

            printf("Sending text...%s\n",line_buf);
            chan->broadcast(algorithm, (unsigned char *)line_buf, line_buf_filled_len);

        } else if (strcmp(line_buf, "send file") == 0 || strcmp(line_buf, "f") == 0) {
            //Get the desired broadcast algorithm
            algorithm = get_alg();
            if (algorithm == QUIT) continue;

            //Get the filename
            printf("Which File?\n");
            read_stripped_line();

            //Open the file
            FILE *fp = fopen(line_buf, "r");
            if(fp == NULL){
                glob_log.log(1, "Could not read file %s: %s\n", line_buf, strerror(errno));
                continue;
            }

            //Get the file length
            struct stat fp_stat;
            if(-1 == fstat(fileno(fp), &fp_stat)){
                error(-1, errno, "Error statting '%s'", line_buf);
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

        } else if (strcmp(line_buf, "debug-toggle") == 0 || strcmp(line_buf, "d") == 0) {
            printf("Debug mode is now %s\n", (chan->toggle_debug_mode()) ? "on" : "off");

        } else if (strcmp(line_buf, "log-level") == 0 || strcmp(line_buf, "l") == 0) {
            printf("Input new log level [1=quiet, 2=chatty, 3=verbose]: ");
            read_stripped_line();
            int new_level = strtol(line_buf, NULL, 10);
            glob_log.set_level(new_level);

        } else if (strcmp(line_buf, "spin") == 0 || strcmp(line_buf, "s") == 0) {
            printf("Input spin time: ");
            read_stripped_line();
            unsigned seconds = (unsigned)strtol(line_buf, NULL, 10);
            while(sleep(seconds)) ;
        } else if (strcmp(line_buf, "zombify") == 0 || strcmp(line_buf, "z") == 0) {
            printf("Turing into a zooOOoombie!\n");
            while(true)
                sleep(100);
        } else if (strcmp(line_buf, "message-history") == 0 || strcmp(line_buf, "m") == 0) {
            printf("Not yet implemented. :-(\n");
        } else {
            printf("Invalid command: %s\n", line_buf);
        }
    }
}


int main(int argc, char *argv[]){
    if(argc != 3){
        usage();
        exit(EINVAL);
    }

    //Initialize the client
    Client c(argv[1], argv[2]);

    // Connect to the group
    c.connect();

    // Run the command line interface
    c.run_cli();

    return 0;
}

