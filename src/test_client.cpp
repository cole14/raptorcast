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

TestClient::TestClient(std::string mp, int gp, bool mst)
:chan(NULL)
,name()
,groupport(gp)
,master(mst)
{
    if(master)
        name = "master";
    else{
        srandom((unsigned)time(NULL));
        //Add some randomness to the name
        name = "drone" + std::to_string(random());
    }

    //Create the broadcast channel
    chan = new Broadcast_Channel(name, mp, this);
}

TestClient::~TestClient(void){
}

void TestClient::test(void){
    if(master){
        //Create the group
        printf("I'm the master!\n");
        chan->join("", 0);

        //Run the test
        while(true){
            char *line = NULL;
            size_t n = 0;

            //Get the action to perform
            if(-1 == getline(&line, &n, stdin))
                error(-1, errno, "Error reading user input");
            //Test iteratively
            if(!strncmp(line, "i", 1)){
                msg_t type;

                //Get the message type to test
                if(-1 == getline(&line, &n, stdin))
                    error(-1, errno, "Error reading msg type");
                if(!strncmp(line, "c", 1))
                    type = CLIENT_SERVER;
                else if(!strncmp(line, "t", 1))
                    type = TRAD;
                else if(!strncmp(line, "l", 1))
                    type = LT;

                //Get the number of powers of 2 to test (starting at 2^7)
                if(-1 == getline(&line, &n, stdin))
                    error(-1, errno, "Error reading increment nums");
                int num_iters = std::stoi(line);

                //Test
                size_t msg_size = 256;
                for(int i = 0; i < num_iters; i++){
                    unsigned char *msg = (unsigned char *)malloc(msg_size);
                    chan->broadcast(type, msg, msg_size);
                    free(msg);
                    usleep(2000000);
                    msg_size *= 2;
                }
            //Print test results
            }else if(!strncmp(line, "p", 1)){
                chan->print_message_bandwidth();
            }
        }
    }else{
        //Connect to the test group
        chan->join("localhost", groupport);

        //We're a drone...
        //Sleep foreeeeeever.
        while(true)
            usleep(10000);
    }
}

/*
 * Prints the usage statement.
 */
static void usage(void){
    fprintf(stderr, "Usage: %s <myport> <groupport> <yes|no>\n", program_invocation_short_name);
}

int main(int argc, char **argv){
    if(argc != 4){
        usage();
        exit(-1);
    }

    bool master = false;
    if(!strncmp(argv[3], "yes", 3))
        master = true;
    else
        master = false;

    //Start the test client
    TestClient tc(argv[1], std::stoi(argv[2]), master);

    //Wait until we're done testing
    tc.test();

    return 0;
}
