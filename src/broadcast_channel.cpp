#include <error.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>

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
    my_info.name = name;
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
    fprintf(stdout, "Successfully created broadcast channel listening on %s:%d (", local_h_name, ntohs(my_info.ip.sin_port));
    getnameinfo((sockaddr*)&my_info.ip, sizeof(my_info.ip), local_h_name, 256, NULL, 0, NI_NUMERICHOST);
    fprintf(stdout, "%s:%d)\n", local_h_name, ntohs(my_info.ip.sin_port));

}

void broadcast_channel::connect(std::string& ip_port){
}

void broadcast_channel::send(unsigned char *buf, size_t buf_len){
}

