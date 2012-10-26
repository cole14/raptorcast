#include <error.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "broadcast_channel.h"

//gethostbyname error num
extern int h_errno;
extern int errno;

//Default destructor - nothing to do
broadcast_channel::~broadcast_channel(void)
{ }

//Default constructor - initialize the port member
broadcast_channel::broadcast_channel(std::string name, int port, channel_listener *lstnr){
    //Init members
    listener = lstnr;
    msg_counter = 0;

    //Initialize the client info struct
    if (name.size() > MAX_NAME_LEN)
        error(-1, EINVAL, "Chosen name (%s) is too long (max %d)",
                name.c_str(), MAX_NAME_LEN);
    strcpy(my_info.name, name.c_str());
    my_info.id = 0;

    //Get the hostname of the local host
    char local_h_name[256];
    if(0 != gethostname(local_h_name, 256)){
        error(-1, errno, "Unable to gethostname");
    }

    //Set the client info ip
    struct addrinfo *local_h;
    if(0 != getaddrinfo(local_h_name, NULL, NULL, &local_h)){
        error(-1, h_errno, "Unable to get localhost host information");
    }
    my_info.ip = *((sockaddr_in *)local_h->ai_addr);
    my_info.ip.sin_port = htons(port);

    //delete the local_h linked list
    freeaddrinfo(local_h);

    //for debugging purposes:
    getnameinfo((sockaddr*)&my_info.ip, sizeof(my_info.ip), local_h_name, 256, NULL, 0, NI_NOFQDN);
    fprintf(stdout, "Successfully initialized broadcast channel associated on %s:%d (", local_h_name, ntohs(my_info.ip.sin_port));
    getnameinfo((sockaddr*)&my_info.ip, sizeof(my_info.ip), local_h_name, 256, NULL, 0, NI_NUMERICHOST);
    fprintf(stdout, "%s:%d)\n", local_h_name, ntohs(my_info.ip.sin_port));

}

bool broadcast_channel::join(std::string hostname, int port){
    if (hostname.empty()) {
        // Start new broadcast group
        my_info.id = 1;

    } else {
        // Join existing broadcast group via a known member (the "strap")
        int sock;
        struct sockaddr_in strap_addr;
        struct message join_msg;

        // Setup the socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            error(-1, errno, "Could not open bootstrap socket.");

        // Put together the strap address
        memset(&strap_addr, 0, sizeof(strap_addr));
        strap_addr.sin_family = AF_INET;
        strap_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
        strap_addr.sin_port = port;

        if (connect(sock, (struct sockaddr *) &strap_addr, sizeof(strap_addr)) < 0)
            error(-1, errno, "Could not connect to bootstrap socket.");

        // Construct the outgoing message
        memset(&join_msg, 0, sizeof(join_msg));
        join_msg.type = JOIN;
        join_msg.cli_id = 0; // We don't yet know our client id
        join_msg.msg_id = 0; // Join is always message 0
        join_msg.data_len = sizeof(my_info);
        memcpy(&join_msg.data, &my_info, sizeof(my_info));

        // Send!
        fprintf(stdout, "Socket setup complete, sending join request...\n");
        if (send(sock, &join_msg, sizeof(join_msg), 0) != sizeof(join_msg))
            error(-1, errno, "Could not send bootstrap request.");

        // Wait for response
        // First message will be info about the local host
        int bytes_recieved;
        struct message strap_msg;
        if ((bytes_recieved = recv(sock, &strap_msg, sizeof(strap_msg), 0)) < 0)
            error(-1, errno, "Could not recieve bootstrap response.");
        if (strap_msg.type != PEER)
            error(-1, EIO, "Recieved bad bootstrap response message type.");
        // Following 0 or more messages will network peers


    }
    return false;
}

void broadcast_channel::broadcast(unsigned char *buf, size_t buf_len){
}

