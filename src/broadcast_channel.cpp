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
    struct sockaddr_in strap_addr;
    struct message in_msg, out_msg;
    struct client_info *peer_info;

    // Setup the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error(-1, errno, "Could not create bootstrap socket.");

    // Put together the strap address
    memset(&strap_addr, 0, sizeof(strap_addr));
    strap_addr.sin_family = AF_INET;
    strap_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
    strap_addr.sin_port = port;

    if (connect(sock, (struct sockaddr *) &strap_addr, sizeof(strap_addr)) < 0)
        error(-1, errno, "Could not connect to bootstrap socket.");

    // Construct the outgoing message
    memset(&out_msg, 0, sizeof(out_msg));
    out_msg.type = JOIN;
    out_msg.cli_id = 0; // We don't yet know our client id
    out_msg.msg_id = msg_counter++;
    out_msg.data_len = sizeof(my_info);
    memcpy(&out_msg.data, &my_info, sizeof(my_info));

    // Send!
    fprintf(stdout, "Socket setup complete, sending join request...\n");
    if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
        error(-1, errno, "Could not send bootstrap request.");

    // Wait for response
    // First message will be info about us as seen by strap
    if ((bytes_recieved = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
        error(-1, errno, "Could not recieve bootstrap response.");
    if (in_msg.type != PEER)
        error(-1, EIO, "Recieved bad bootstrap response message type.");

    peer_info = (struct client_info *) in_msg.data;
    if (strcmp(peer_info->name, my_info.name) != 0)
        error(-1, EIO, "Bootstrap response gave bad name.");
    my_info.id = peer_info->id;

    fprintf(stdout, "Recieved response from bootstrap!  Reading peer list\n");

    // Following 0 or more messages will represent network peers
    while (recv(sock, &in_msg,sizeof(in_msg), 0) > 0) {
        if (in_msg.type != PEER)
            error(-1, EIO, "Bootstrap sent non-peer message.");
        peer_info = (struct client_info *) malloc(sizeof(client_info));
        memcpy(peer_info, in_msg.data, sizeof(peer_info));
        group_set.push_back(peer_info);

        fprintf(stdout, "Read peer info!  Name %s, host %s, port %d\n",
                peer_info->name,
                inet_ntoa(peer_info->ip.sin_addr),
                peer_info->ip.sin_port);
    }

    // Clean up
    if (close(sock) != 0)
        error(-1, errno, "Error closing bootstrap socket.");

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
        error(-1, EINVAL, "The peer list is empty!");

    fprintf(stdout, "Notifying %d peers of presence...\n", (int)group_set.size());

    // Setup the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error(-1, errno, "Could not create bootstrap socket.");

    // Message each peer and let them know we're here.
    // XXX Haven't ever used any std stuff before, so this is probably
    // terrible.
    for (int i = 0; i < (int) group_set.size(); i++) {
        peer =  group_set[i];
        fprintf(stdout, "Notifying peer %s.", peer->name);

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s.", peer->name);

        // Construct the outgoing message
        memset(&out_msg, 0, sizeof(out_msg));
        out_msg.type = PEER;
        out_msg.cli_id = my_info.id;
        out_msg.msg_id = msg_counter++;
        out_msg.data_len = sizeof(my_info);
        memcpy(&out_msg.data, &my_info, sizeof(my_info));

        // Send!
        fprintf(stdout, "Sending...\n");
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification.");

        // Get reply
        if ((bytes_recieved = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
            error(-1, errno, "Could not recieve peer response.");
        if (in_msg.type != READY)
            error(-1, errno, "Recieved bad peer response message type.");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket.");
    }

    return true;
}

bool broadcast_channel::send_peer_list(int client_sock, struct client_info *target) {
    return false;
}

bool broadcast_channel::accept_connections() {
    int server_sock, client_sock;
    struct sockaddr_in client_addr;
    int bytes_recieved;
    struct message in_message, out_message;
    struct client_info *peer_info;

    // Set up the socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error(-1, errno, "Could not create server socket.");
    if (bind(server_sock, (struct sockaddr *) &my_info.ip, sizeof(my_info.ip)) < 0)
        error(-1, errno, "Could not bind to socket.");

    listen(server_sock, 10);

    while(1) {
        fprintf(stdout, "Server waiting...\n");

        // Wait for a connection
        client_sock = accept(server_sock, (struct sockaddr *) &client_addr, sizeof(client_addr), 0);
        if (client_sock < 0)
            error(-1, errno, "Could not accept client.");
        fprintf(stdout, "accepted client!\n");

        // Handle the request
        if ((bytes_recieved = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
            error(-1, errno, "Could not recieve client message.");

        switch (in_msg.type) {
            case JOIN :
                send_peer_list(client_sock, &in_msg);
                break;
            case PEER:
                // Add the new peer to the group
                peer_info = (struct client_info *) malloc(sizeof(client_info));
                memcpy(peer_info, in_msg.data, sizeof(peer_info));
                group_set.push_back(peer_info);

                fprintf(stdout, "Recieved peer request!  Name %s, host %s, port %d\n",
                        peer_info->name,
                        inet_ntoa(peer_info->ip.sin_addr),
                        peer_info->ip.sin_port);

                // Send our READY reply
                memset(&out_msg, 0, sizeof(out_msg));
                out_msg.type = READY;
                out_msg.cli_id = my_info.id;
                out_msg.msg_id = msg_counter++;

                fprintf(stdout, "Sending READY reply...\n");
                if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
                    error(-1, errno, "Could not send peer notification.");

                // Close socket
                if (close(client_sock) != 0)
                    error(-1, errno, "Error closing peer socket.");

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
    }
}


bool broadcast_channel::join(std::string hostname, int port){
    if (hostname.empty()) {
        // Start new broadcast group
        my_info.id = 1;

    } else {
        // Join an existing broadcast group
        if (! get_peer_list(hostname, port))
            error(-1, errno, "Could not get peer list!");

        if (! notify_peers())
            error(-1, errno, "Could not notify peers!");

    }

    // TODO: Start of a channel listener

    return false;
}

void broadcast_channel::broadcast(unsigned char *buf, size_t buf_len){
}

