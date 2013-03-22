//Allow use of 'program_invocation_short_name'
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <error.h>
#include <stdlib.h>
#include <string.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include "logger.h"

#include "test_client.h"

void TestClient::receive(unsigned char *buf, size_t buf_len){
    printf("Received message of length %zu\n", buf_len);
}

TestClient::TestClient(const char *config_filename){
    //Open the config file
    FILE *fp = fopen(config_filename, "r");
    if(fp == NULL){
        error(-1, errno, "Unable to open configuration file");
    }

    std::string port;

    //Parse the config file
    char *line = NULL;
    size_t n = 0;
    while(-1 != getline(&line, &n, fp)){
        if(!strncmp(line, "Name:", 5)){
            //Read the name
            if(-1 == getline(&line, &n, fp))
                error(-1, errno, "Error reading config file");
            name = std::string(line);
        }else if(!strncmp(line, "Port:", 5)){
            //Read the port
            if(-1 == getline(&line, &n, fp))
                error(-1, errno, "Error reading config file");
            port = std::string(line);
        }
    }
    //Trim the strings
    name.erase(std::find_if(name.rbegin(), name.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), name.end());
    port.erase(std::find_if(port.rbegin(), port.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), port.end());

    //Add some randomness to the name
    name += std::to_string(random());

    //Create the broadcast channel
    chan = new Broadcast_Channel(name, port, this);
}

TestClient::~TestClient(void){
}

void TestClient::test(void){
    //TODO: We should implement a planned exit in the future.
    //      Maybe after X receives (config param) we just exit.
    //Currently just sleep forever.
    while(true)
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
}

/*
 * Prints the usage statement.
 */
static void usage(void){
    fprintf(stderr, "Usage: %s <configuration-file>\n", program_invocation_short_name);
}

int main(int argc, char **argv){
    if(argc != 2){
        usage();
        exit(-1);
    }

    //Start the test client
    TestClient tc(argv[1]);

    //Wait until we're done testing
    tc.test();

    return 0;
}
