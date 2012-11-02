#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "broadcast_channel.h"

//gethostbyname error num
extern int h_errno;
extern int errno;

//Default destructor - nothing to do
broadcast_channel::~broadcast_channel(void)
{
    //Delete the group_set
    for(std::vector< struct client_info * >::iterator it = group_set.begin(); it != group_set.end(); it++){
        free(*it);
    }
}

//Default constructor - initialize the port member
broadcast_channel::broadcast_channel(std::string name, int port, channel_listener *lstnr){
    //Init members
    listener = lstnr;
    msg_counter = 0;

    //Allocate the client_info struct
    my_info = (struct client_info *)calloc(1, sizeof(struct client_info));

    //Initialize the client info struct
    if (name.size() > MAX_NAME_LEN)
        error(-1, EINVAL, "Chosen name (%s) is too long (max %d)",
                name.c_str(), MAX_NAME_LEN);
    strcpy(my_info->name, name.c_str());
    my_info->id = 0;

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
    my_info->ip = *((sockaddr_in *)local_h->ai_addr);
    my_info->ip.sin_port = htons(port);

    //delete the local_h linked list
    freeaddrinfo(local_h);

    //for debugging purposes:
    getnameinfo((sockaddr*)&(my_info->ip), sizeof(my_info->ip), local_h_name, 256, NULL, 0, NI_NOFQDN);
    fprintf(stdout, "Successfully initialized broadcast channel associated on %s:%d (", local_h_name, ntohs(my_info->ip.sin_port));
    getnameinfo((sockaddr*)&(my_info->ip), sizeof(my_info->ip), local_h_name, 256, NULL, 0, NI_NUMERICHOST);
    fprintf(stdout, "%s:%d)\n", local_h_name, ntohs(my_info->ip.sin_port));

}

void broadcast_channel::print_peers(int indent) {
    struct client_info *peer;
    for (unsigned int i = 0; i < group_set.size(); i++) {
        peer = group_set[i];
        for(int i = 0; i < indent; i++) printf("%s", "    ");
        printf("Peer %d: %s -> %s:%d\n",
                peer->id,
                peer->name,
                inet_ntoa(peer->ip.sin_addr),
                ntohs(peer->ip.sin_port));
    }
}


void broadcast_channel::construct_message(msg_t type, struct message *dest, const void *src, size_t n) {
    memset(dest, 0, sizeof(&dest));
    dest->type = type;
    dest->cli_id = my_info->id;
    dest->msg_id = msg_counter++;
    dest->data_len = n;
    memcpy(&dest->data, src, n);
}

/*
 * Request the group set from a known host (the "strap")
 * Sends a JOIN message to hostname:port, containing information about this
 * host.  The strap should respond with a PEER message containing information
 * about us complete with ID, and then 1 or more PEER messages containing info
 * on the other peers in the network.
 */
bool broadcast_channel::get_peer_list(std::string hostname, int port) {
    int sock;
    int bytes_recieved;
    struct addrinfo *strap_h;
    struct sockaddr_in *strap_addr;
    struct message in_msg, out_msg;
    struct client_info *peer_info;

    // Setup the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error(-1, errno, "Could not create bootstrap socket");

    // Put together the strap address
    if (0 != getaddrinfo(hostname.c_str(), NULL, NULL, &strap_h))
        error(-1, h_errno, "Unable to resolve bootstrap host");
    strap_addr = (sockaddr_in *)strap_h->ai_addr;
    strap_addr->sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) strap_addr, sizeof(struct sockaddr_in)) < 0)
        error(-1, errno, "Could not connect to bootstrap socket");

    // Construct the outgoing message
    construct_message(JOIN, &out_msg, my_info, sizeof(my_info));

    // Send!
    if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
        error(-1, errno, "Could not send bootstrap request");

    // Wait for response
    // First message will be info about us as seen by strap
    if ((bytes_recieved = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
        error(-1, errno, "Could not recieve bootstrap response");
    if (in_msg.type != PEER)
        error(-1, EIO, "Recieved bad bootstrap response message type");

    peer_info = (struct client_info *) in_msg.data;
    if (strcmp(peer_info->name, my_info->name) != 0)
        error(-1, EIO, "Bootstrap response gave bad name");
    my_info->id = peer_info->id;

    // Following 0 or more messages will represent network peers
    while (recv(sock, &in_msg,sizeof(in_msg), 0) > 0) {
        if (in_msg.type != PEER)
            error(-1, EIO, "Bootstrap sent non-peer message");
        peer_info = (struct client_info *) malloc(sizeof(client_info));
        memcpy(peer_info, in_msg.data, sizeof(struct client_info));
        group_set.push_back(peer_info);

        fprintf(stdout, "Read peer info!  Name %s, host %s, port %d\n",
                peer_info->name,
                inet_ntoa(peer_info->ip.sin_addr),
                ntohs(peer_info->ip.sin_port));
    }

    // Clean up
    if (close(sock) != 0)
        error(-1, errno, "Error closing bootstrap socket");

    freeaddrinfo(strap_h);

    return true;
}

/*
 * Send a PEER message containing information about us to everyone on the peer
 * list.  They will respond with a READY, indicating that they have added us to
 * their peer list, and are OK to recieve messages from us.
 */
bool broadcast_channel::notify_peers() {
    int sock;
    int bytes_recieved;
    struct message in_msg, out_msg;
    struct client_info *peer;

    if (group_set.size() <= 0)
        error(-1, EINVAL, "The peer list is empty");

    // Message each peer and let them know we're here.
    // When this is called, we haven't yet added ourself to the peer list,
    // so no check is needed
    for (int i = 0; i < (int) group_set.size(); i++) {
        peer =  group_set[i];
        fprintf(stdout, "Notifying peer %s.\n", peer->name);

        // Setup the socket
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
            error(-1, errno, "Could not create bootstrap socket");

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", peer->name);

        // Construct the outgoing message
        construct_message(PEER, &out_msg, my_info, sizeof(my_info));

        // Send!
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Get reply
        if ((bytes_recieved = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
            error(-1, errno, "Could not recieve peer response");
        if (in_msg.type != READY)
            error(-1, errno, "Recieved bad peer response message type");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");
    }

    return true;
}

unsigned int broadcast_channel::get_unused_id() {
    unsigned int max_used = 0;
    for (int i = 0; i < (int) group_set.size(); i++)
        if (group_set[i]->id > max_used) max_used = group_set[i]->id;
    return max_used + 1;
}

bool broadcast_channel::send_peer_list(int sock, struct client_info *target) {
    struct message out_msg;
    struct client_info *peer;

    // First, send info about the way we see the target
    target->id = get_unused_id();

    // Construct the outgoing message
    construct_message(PEER, &out_msg, target, sizeof(struct client_info));

    // Send!
    if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
        error(-1, errno, "Could not send peer notification");

    // Next, send the current list of peers
    for (int i = 0; i < (int) group_set.size(); i++) {
        peer =  group_set[i];
        fprintf(stdout, "Sending info for peer %s.\n", peer->name);

        // Construct the outgoing message
        construct_message(PEER, &out_msg, peer, sizeof(struct client_info));

        // Send!
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");
    }

    return true;
}

void *broadcast_channel::start_server(void *args) {
    broadcast_channel *bc = (broadcast_channel *) args;
    bc->accept_connections();
    return NULL;
}

void broadcast_channel::accept_connections() {
    int server_sock, client_sock;
    int bytes_recieved;
    struct message in_msg, out_msg;
    struct client_info *peer_info;
    struct sockaddr_in servaddr;

    // Set up the socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error(-1, errno, "Could not create server socket");

    // Put together a sockaddr for us.  Note that the only difference between this and my_info
    // is that my_info's sin_addr represents this IP address, whereas we want INADDR_ANY
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = my_info->ip.sin_port;
    if (bind(server_sock, (struct sockaddr *) &servaddr, sizeof(my_info->ip)) < 0)
        error(-1, errno, "Could not bind to socket");

    listen(server_sock, 10);

    while(1) {
        fprintf(stdout, "Server waiting...\n");

        // Wait for a connection
        client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0)
            error(-1, errno, "Could not accept client.");
        fprintf(stdout, "accepted client!\n");

        // Handle the request
        if ((bytes_recieved = recv(client_sock, &in_msg, sizeof(in_msg), 0)) < 0)
            error(-1, errno, "Could not recieve client message");

        switch (in_msg.type) {
            case JOIN :
                send_peer_list(client_sock, (struct client_info *)&in_msg.data);
                break;

            case PEER:
                // Add the new peer to the group
                peer_info = (struct client_info *) malloc(sizeof(client_info));
                memcpy(peer_info, &in_msg.data, sizeof(struct client_info));
                group_set.push_back(peer_info);

                fprintf(stdout, "Recieved peer request!  Name %s, host %s, port %d\n",
                        peer_info->name,
                        inet_ntoa(peer_info->ip.sin_addr),
                        ntohs(peer_info->ip.sin_port));

                // Send our READY reply
                memset(&out_msg, 0, sizeof(out_msg));
                out_msg.type = READY;
                out_msg.cli_id = my_info->id;
                out_msg.msg_id = msg_counter++;

                if (send(client_sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
                    error(-1, errno, "Could not send ready reply");

                break;

            case QUIT:
                // Not implemented
                break;

            case CLIENT_SERVER:
            case TRAD:
            case COOP:
            case RAPTOR:
                // Not implemented
                break;

            default:
                break;
        }

        // Close socket
        if (close(client_sock) != 0)
            error(-1, errno, "Error closing peer socket");

    }
}


bool broadcast_channel::join(std::string hostname, int port){
    if (hostname.empty()) {
        // Start new broadcast group
        my_info->id = 1;

    } else {
        // Join an existing broadcast group
        if (! get_peer_list(hostname, port))
            error(-1, errno, "Could not get peer list");

        if (! notify_peers())
            error(-1, errno, "Could not notify peers");

    }

    // Add myself to the peer list (we create a copy to make cleanup easier)
    group_set.push_back(my_info);

    pthread_t listen_thread;
    pthread_create( &listen_thread, NULL, start_server, (void *)this);

    return true;
}

void broadcast_channel::broadcast(unsigned char *buf, size_t buf_len){
}

