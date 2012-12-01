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

void dump_buf(unsigned char *buf, size_t len){
    for(int i = 0; i < (int)len; i++){
        fprintf(stdout, "%02X ", buf[i]);
        if((i+1) % 16 == 0)
            fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
}

const char * msg_t_to_str(msg_t type) {
    switch (type) {
        case JOIN:
            return "JOIN";
        case PEER:
            return "PEER";
        case READY:
            return "READY";
        case QUIT:
            return "QUIT";
        case CLIENT_SERVER:
            return "CLIENT_SERVER";
        case TRAD:
            return "TRAD";
        case COOP:
            return "COOP";
        case RAPTOR:
            return "RAPTOR";
        default:
            return "UNKNOWN MESSAGE TYPE";
    }
}

const char *cli_to_str(struct client_info *cli) {
    const int max_size = 256;
    static char str[max_size];
    snprintf(str, max_size, "%u:%s at %s:%d",
            cli->id,
            cli->name,
            inet_ntoa(cli->ip.sin_addr),
            ntohs(cli->ip.sin_port));
    return str;
}

broadcast_channel::~broadcast_channel(void)
{
    //Delete the group_set
    for(std::vector< struct client_info * >::iterator it = group_set.begin(); it != group_set.end(); it++){
        free(*it);
    }
}

//Default constructor - initialize the port member
broadcast_channel::broadcast_channel(std::string name, std::string port, channel_listener *lstnr){
    //Init members
    listener = lstnr;
    msg_counter = 0;
    debug_mode = false;

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
    if(0 != getaddrinfo(local_h_name, port.c_str(), NULL, &local_h)){
        error(-1, h_errno, "Unable to get localhost host information");
    }
    my_info->ip = *((sockaddr_in *)local_h->ai_addr);

    //Make sure we can use the supplied info and set up the
    //chunk receiver socket (don't start listenening though).
    server_sock = make_socket();

    // Put together a sockaddr for us.  Note that the only difference
    // between this and my_info is that my_info's sin_addr represents
    // this IP address, whereas we want INADDR_ANY
    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = my_info->ip.sin_port;
    if (bind(server_sock, (struct sockaddr *) &servaddr, sizeof(my_info->ip)) < 0)
        error(-1, errno, "Could not bind to supplied local port %d", ntohs(my_info->ip.sin_port));

    //delete the local_h linked list
    freeaddrinfo(local_h);

    //for debugging purposes:
    getnameinfo((sockaddr*)&(my_info->ip), sizeof(my_info->ip),
            local_h_name, 256, NULL, 0, NI_NOFQDN);
    fprintf(stdout, "Successfully initialized broadcast channel associated on %s:%d (",
            local_h_name, ntohs(my_info->ip.sin_port));
    getnameinfo((sockaddr*)&(my_info->ip), sizeof(my_info->ip),
            local_h_name, 256, NULL, 0, NI_NUMERICHOST);
    fprintf(stdout, "%s:%d)\n", local_h_name, ntohs(my_info->ip.sin_port));

}

client_info *broadcast_channel::get_peer_by_id(unsigned int id) {
    for (unsigned int i = 0; i < group_set.size(); i++) {
        if (group_set[i]->id == id) {
            return group_set[i];
        }
    }
    return NULL;
}

bool broadcast_channel::toggle_debug_mode(){
    return (debug_mode = !debug_mode);
}

int broadcast_channel::make_socket(){
    int arg = 1;
    int sock;

    //Make the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        error(-1, errno, "Could not create socket");
    //Set the options
    if (0 != setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &arg, sizeof(arg)))
        error(-1, errno, "Could not set up socket options");
    if (0 != setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)))
        error(-1, errno, "Could not set up socket options");

    return sock;
}

void broadcast_channel::construct_message(msg_t type, struct message *dest, const void *src, size_t n) {
    memset(dest, 0, sizeof(&dest));
    dest->type = type;
    dest->cli_id = my_info->id;
    dest->msg_id = msg_counter++;
    dest->chunk_id = 0;
    if (src != NULL) {
        dest->data_len = n;
        memcpy(&dest->data, src, n);
    } else {
        dest->data_len = 0;
    }
}

void broadcast_channel::print_peers(int indent) {
    struct client_info *peer;
    for (unsigned int i = 0; i < group_set.size(); i++) {
        peer = group_set[i];
        for(int i = 0; i < indent; i++) printf("%s", "    ");
        printf("Peer %s\n", cli_to_str(peer));
    }
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
    int bytes_received;
    struct addrinfo *strap_h;
    struct sockaddr_in *strap_addr;
    struct message in_msg, out_msg;

    // Setup the socket
    sock = make_socket();

    // Put together the strap address
    if (0 != getaddrinfo(hostname.c_str(), NULL, NULL, &strap_h))
        error(-1, h_errno, "Unable to resolve bootstrap host");
    strap_addr = (sockaddr_in *)strap_h->ai_addr;
    strap_addr->sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) strap_addr, sizeof(struct sockaddr_in)) < 0)
        error(-1, errno, "Could not connect to bootstrap socket");

    // Construct the outgoing message
    construct_message(JOIN, &out_msg, my_info, sizeof(struct client_info));

    // Send!
    if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
        error(-1, errno, "Could not send bootstrap request");

    // Wait for response
    // First message will be info about us as seen by strap
    if ((bytes_received = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
        error(-1, errno, "Could not receive bootstrap response");
    if (in_msg.type != PEER)
        error(-1, EIO, "received bad bootstrap response message type");

    struct client_info *peer_info;
    peer_info = (struct client_info *) in_msg.data;
    if (strcmp(peer_info->name, my_info->name) != 0)
        error(-1, EIO, "Bootstrap response gave bad name");
    fprintf(stdout, "Got ID %u from bootstrap\n", peer_info->id);
    my_info->id = peer_info->id;

    // Following 0 or more messages will represent network peers
    while (recv(sock, &in_msg,sizeof(in_msg), 0) > 0) {
        add_peer(&in_msg);
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
 * their peer list, and are OK to receive messages from us.
 */
bool broadcast_channel::notify_peers() {
    int sock;
    int bytes_received;
    struct message in_msg, out_msg;
    struct client_info *peer;

    if (group_set.size() <= 0)
        error(-1, EINVAL, "The peer list is empty");

    // Message each peer and let them know we're here.
    // When this is called, we haven't yet added ourself to the peer list,
    // so no check is needed
    for (int i = 0; i < (int) group_set.size(); i++) {
        peer =  group_set[i];
        fprintf(stdout, "Notifying peer %s.\n", cli_to_str(peer));

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        // Construct the outgoing message
        construct_message(PEER, &out_msg, my_info, sizeof(struct client_info));

        // Send!
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Get reply
        if ((bytes_received = recv(sock, &in_msg, sizeof(in_msg), 0)) < 0)
            error(-1, errno, "Could not receive peer response");
        if (in_msg.type != READY)
            error(-1, errno, "received bad peer response message type");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");
    }

    return true;
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
        fprintf(stdout, "Sending info for peer %s.\n", cli_to_str(peer));

        // Construct the outgoing message
        construct_message(PEER, &out_msg, peer, sizeof(struct client_info));

        // Send!
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");
    }

    return true;
}

void broadcast_channel::add_peer(struct message *in_msg) {
    if (in_msg->type != PEER)
        error(-1, EIO, "Tried to add peer from non-peer message");

    struct client_info *peer_info;
    peer_info = (struct client_info *) malloc(sizeof(client_info));
    memcpy(peer_info, &in_msg->data, sizeof(struct client_info));
    group_set.push_back(peer_info);

    fprintf(stdout, "received peer request! Peer %s\n", cli_to_str(peer_info));
}

unsigned int broadcast_channel::get_unused_id() {
    unsigned int max_used = 0;
    for (int i = 0; i < (int) group_set.size(); i++)
        if (group_set[i]->id > max_used) max_used = group_set[i]->id;
    return max_used + 1;
}

void *broadcast_channel::start_server(void *args) {
    broadcast_channel *bc = (broadcast_channel *) args;
    bc->accept_connections();
    return NULL;
}

void broadcast_channel::accept_connections() {
    int client_sock;
    int bytes_received;
    struct message in_msg, out_msg;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    struct client_info *peer_info;


    //Start listening on the chunk receiver socket
    listen(server_sock, 10);

    bool running = true;
    while (running) {
        fprintf(stdout, "Server waiting...\n");

        // Wait for a connection
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
            error(-1, errno, "Could not accept client.");
        fprintf(stdout, "accepted client!\n");

        // Handle the request
        if ((bytes_received = recv(client_sock, &in_msg, sizeof(in_msg), 0)) < 0)
            error(-1, errno, "Could not receive client message");

        switch (in_msg.type) {
            case JOIN :
                send_peer_list(client_sock, (struct client_info *)&in_msg.data);
                break;

            case PEER:
                // Add the new peer to the group
                add_peer(&in_msg);
                // Send our READY reply
                construct_message(READY, &out_msg, NULL, 0);

                if (send(client_sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
                    error(-1, errno, "Could not send ready reply");
                break;

            case QUIT:
                peer_info = (struct client_info *) in_msg.data;
                fprintf(stdout, "received quit message from %s\n", cli_to_str(peer_info));
                if (peer_info->id == my_info->id) {
                    // Shutdown message from the CLI thread
                    fprintf(stdout, "Shutting down receive thread\n");
                    running = false;
                    break;
                }

                int index;
                for (index = 0; index < (int) group_set.size(); index++) {
                    if (group_set[index]->id == peer_info->id)
                        break;
                }

                if (index < (int) group_set.size()) {
                    group_set.erase(group_set.begin() + index);
                } else {
                    fprintf(stderr, "received quit notice from an unknown peer: %s",
                            cli_to_str(peer_info));
                }
                break;

            case CLIENT_SERVER:
            case COOP:
                handle_chunk(client_sock, &in_msg);

                break;
            case TRAD:
            case RAPTOR:
                // Not implemented
                fprintf(stderr, "Oh Noes! We got a message we don't know what to do with...\n");

            default:
                break;
        }

        // Close socket
        if (close(client_sock) != 0)
            error(-1, errno, "Error closing peer socket");

    }
}

void broadcast_channel::handle_chunk(int client_sock, struct message *in_msg) {
    struct message *msg_list = NULL;
    size_t num_msg = 1;
    decoder *msg_dec = NULL;
    size_t bytes_received = 0;

    if (in_msg->cli_id == my_info->id)
        return;  // It's just a bump of one of our own messages

    // Keep track of the messages we've gotten, so we can forward them along
    msg_list = (struct message *)realloc(msg_list, sizeof(struct message));
    memcpy(&msg_list[0], in_msg, sizeof(struct message));

    fprintf(stdout, "received %s message\n", msg_t_to_str(in_msg->type));

    // Get a decoder for this message
    if(decoders.find(in_msg->msg_id) == decoders.end()){
        msg_dec = get_decoder(in_msg->type);
        decoders[in_msg->msg_id] = msg_dec;
    }
    msg_dec = decoders[in_msg->msg_id];

    // Add the chunk we just got
    msg_dec->add_chunk(in_msg->data, in_msg->data_len, in_msg->chunk_id);
    dump_buf(in_msg->data, in_msg->data_len);

    // Keep reading chunks and adding them until the bcast is done
    while (in_msg->data_len != 0) {
        if ((bytes_received = recv(client_sock, in_msg, sizeof(struct message), 0)) < 0)
            error(-1, errno, "Could not receive client message");

        num_msg++;
        msg_list = (struct message *)realloc(msg_list, num_msg * sizeof(struct message));
        memcpy(&msg_list[num_msg-1], in_msg, sizeof(struct message));

        msg_dec->add_chunk(in_msg->data, in_msg->data_len, in_msg->chunk_id);
        fprintf(stdout, "dumping buf\n");
        dump_buf(in_msg->data, in_msg->data_len);
    }

    // Forward the message on to the other peers
    if (msg_dec->should_forward() && msg_list->ttl > 0) {
        forward(msg_list, num_msg);
    }

    // Print the message and clean up
    if(msg_dec->is_done()){
        listener->receive(msg_dec->get_message(), msg_dec->get_len());
        decoders.erase(in_msg->msg_id);
    }
    free(msg_list);
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

    // Add myself to the peer list
    group_set.push_back(my_info);

    pthread_create( &receiver_thread, NULL, start_server, (void *)this);

    return true;
}

void broadcast_channel::quit() {
    int sock;
    struct client_info *peer;
    struct message out_msg;
    fprintf(stdout, "Putting in 2 weeks' notice\n");

    // Notify peers that we're quitting
    // Note: this will include our listener thread - this is good, because
    // it provides an easy way to tell it to shut down.
    for (int i = 0; i < (int)group_set.size(); i++) {
        peer =  group_set[i];
        fprintf(stdout, "Notifying peer %s.\n", cli_to_str(peer));

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        // Construct the outgoing message
        construct_message(QUIT, &out_msg, my_info, sizeof(struct client_info));

        // Send
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");
    }

    pthread_join(receiver_thread, NULL);
}

void broadcast_channel::broadcast(msg_t algo, unsigned char *buf, size_t buf_len){
    int sock;
    struct client_info *peer;

    // Get the msg encoder for the given algorithm type
    encoder *msg_enc = get_encoder(algo);
    if(msg_enc == NULL)
        error(-1, 0, "Unable to get encoder for algorithm type %d", (int)algo);

    // Set up the encoder with the message data and chunk size
    msg_enc->init(buf, buf_len, PACKET_LEN, group_set.size()-1);

    // Increment the message id counter
    msg_counter++;

    // Continually generate chunks until the decoder is out of chunks
    size_t chunk_size = 0;
    unsigned char *chunk = NULL;
    unsigned int chunk_id = 0;
    struct message out_msg;
    for (int i = 0; i < (int)group_set.size(); i++) {
        peer =  group_set[i];

        //Don't broadcast to yourself
        if(peer->id == my_info->id) continue;

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        while((chunk_size = msg_enc->generate_chunk(&chunk, &chunk_id)) > 0 &&
                chunk != NULL){
            printf("Sending chunk %u of msg %lu to peer %u\n",
                    chunk_id, msg_counter, peer->id);
            // Build the message around the chunk
            out_msg.type = algo;
            out_msg.cli_id = my_info->id;
            out_msg.msg_id = msg_counter;//we don't want a new msgid for each chunk
            out_msg.chunk_id = chunk_id;
            out_msg.ttl = 1;
            out_msg.data_len = chunk_size;
            memset(&out_msg.data, 0, PACKET_LEN);
            memcpy(&(out_msg.data), chunk, chunk_size);

            // Send the message
            if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
                error(-1, errno, "Could not send peer notification");

            //free the chunk memory
            free(chunk);
        }

        // Send the final, empty chunk (to let the peer know this cast is finished)
        out_msg.type = algo;
        out_msg.cli_id = my_info->id;
        out_msg.msg_id = msg_counter;//we don't want a new msgid for each chunk
        out_msg.chunk_id = 0;
        out_msg.ttl = 1;
        out_msg.data_len = 0;
        memset(&out_msg.data, 0, PACKET_LEN);

        // Send the message
        if (send(sock, &out_msg, sizeof(out_msg), 0) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");

        // Reset the encoder for the next peer
        msg_enc->next_stream();
    }

    delete msg_enc;
}

/*
 * Forward a list of messages to the rest of the peer group
 */
void broadcast_channel::forward(struct message *msg_list, size_t num_msg) {
    int sock;
    struct client_info *peer;
    struct message *out_msg;
    for (int i = 0; i < (int)group_set.size(); i++) {
        peer =  group_set[i];

        //Don't broadcast to yourself or the origin
        if(peer->id == my_info->id || peer->id == msg_list->cli_id) continue;

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        // Send all messages
        for (out_msg = msg_list; out_msg < msg_list + num_msg; out_msg++) {
            if (out_msg->data_len != 0) {
                printf("Forwarding chunk %u of msg %u from peer %u to peer %u\n",
                        out_msg->chunk_id, out_msg->msg_id,
                        out_msg->cli_id, peer->id);
            } else {
                printf("Forwarding terminal chunk of msg %u "
                        "from peer %u to peer %u\n",
                        out_msg->msg_id,
                        out_msg->cli_id, peer->id);
            }

            out_msg->ttl = 0;  // We don't want people to rebroadcast rebroadcasts
            if (send(sock, out_msg, sizeof(struct message), 0) != sizeof(struct message))
                error(-1, errno, "Could not forward transmission");
        }

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");
    }
}
