#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <utility>

#include "broadcast_channel.h"

#include "logger.h"

//gethostbyname error num
extern int h_errno;
extern int errno;

void dump_buf(int level, void *b, size_t len){
    unsigned char *buf = (unsigned char *)b;
    glob_log.log(level+1, "dumping buf:\n");
    for(int i = 0; i < (int)len; i++){
        glob_log.log(level, "%02X ", buf[i]);
        if((i+1) % 16 == 0)
            glob_log.log(level, "\n");
    }
    glob_log.log(level, "\n");
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
        case CONFIRM:
            return "CONFIRM";
        case CLIENT_SERVER:
            return "CLIENT_SERVER";
        case TRAD:
            return "TRAD";
        case COOP:
            return "COOP";
        case LT:
            return "LT";
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

Broadcast_Channel::~Broadcast_Channel(void)
{
    //Delete the group_set
    for(std::vector< struct client_info * >::iterator it = group_set.begin(); it != group_set.end(); it++){
        free(*it);
    }
}

//Default constructor - initialize the port member
Broadcast_Channel::Broadcast_Channel(std::string name, std::string port, Channel_Listener *lstnr){
    //Init members
    listener = lstnr;
    msg_counter = 0;
    debug_mode = false;

    // Get the clock id for this process
    if (0 != clock_getcpuclockid(0, &clk))
        error(-1, errno, "Unable to get the system clock for timing");

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
    glob_log.log(3, "Successfully initialized broadcast channel associated on %s:%d (",
            local_h_name, ntohs(my_info->ip.sin_port));
    getnameinfo((sockaddr*)&(my_info->ip), sizeof(my_info->ip),
            local_h_name, 256, NULL, 0, NI_NUMERICHOST);
    glob_log.log(3, "%s:%d)\n", local_h_name, ntohs(my_info->ip.sin_port));

}

client_info *Broadcast_Channel::get_peer_by_id(unsigned int id) {
    for (unsigned int i = 0; i < group_set.size(); i++) {
        if (group_set[i]->id == id) {
            return group_set[i];
        }
    }
    return NULL;
}

bool Broadcast_Channel::toggle_debug_mode(){
    return (debug_mode = !debug_mode);
}

int Broadcast_Channel::make_socket(){
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

void Broadcast_Channel::construct_message(msg_t type, struct message *dest, const void *src, size_t n) {
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

ssize_t Broadcast_Channel::read_message(int sock, struct message *msg) {
    ssize_t cur_read = 0;
    size_t tot_read = 0;

    unsigned char *in_msg = (unsigned char *)msg;

    while(tot_read < sizeof(struct message)){
        cur_read = recv(sock, in_msg + tot_read, sizeof(struct message) - tot_read, 0);
        if(cur_read <= 0)
            return cur_read;
        tot_read += (size_t)cur_read;
    }

    return (ssize_t)tot_read;
}

ssize_t Broadcast_Channel::send_message(int sock, struct message *msg) {
    ssize_t cur_sent = 0;
    size_t tot_sent = 0;

    unsigned char *out_msg = (unsigned char *)msg;

    while(tot_sent < sizeof(struct message)){
        cur_sent = send(sock, out_msg + tot_sent, sizeof(struct message) - tot_sent, 0);
        if(cur_sent < 0){
            return cur_sent;
        }
        tot_sent += (size_t)cur_sent;
    }

    return (ssize_t)tot_sent;
}

void Broadcast_Channel::print_peers(int indent) {
    struct client_info *peer;
    for (unsigned int i = 0; i < group_set.size(); i++) {
        peer = group_set[i];
        for(int i = 0; i < indent; i++) glob_log.log(1, "%s", "    ");
        glob_log.log(1, "Peer %s\n", cli_to_str(peer));
    }
}

/*
 * Request the group set from a known host (the "strap")
 * Sends a JOIN message to hostname:port, containing information about this
 * host.  The strap should respond with a PEER message containing information
 * about us complete with ID, and then 1 or more PEER messages containing info
 * on the other peers in the network.
 */
bool Broadcast_Channel::get_peer_list(std::string hostname, int port) {
    int sock;
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
    if (send_message(sock, &out_msg) != sizeof(out_msg))
        error(-1, errno, "Could not send bootstrap request");

    // Wait for response
    // First message will be info about us as seen by strap
    if(read_message(sock, &in_msg) < 0)
        error(-1, errno, "Could not receive bootstrap response");
    if (in_msg.type != PEER)
        error(-1, EIO, "received bad bootstrap response message type");

    struct client_info *peer_info;
    peer_info = (struct client_info *) in_msg.data;
    if (strcmp(peer_info->name, my_info->name) != 0)
        error(-1, EIO, "Bootstrap response gave bad name");
    glob_log.log(3, "Got ID %u from bootstrap\n", peer_info->id);
    my_info->id = peer_info->id;

    // Following 0 or more messages will represent network peers
    while (read_message(sock, &in_msg) > 0) {
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
bool Broadcast_Channel::notify_peers() {
    int sock;
    struct message in_msg, out_msg;
    struct client_info *peer;

    if (group_set.size() <= 0)
        error(-1, EINVAL, "The peer list is empty");

    // Message each peer and let them know we're here.
    // When this is called, we haven't yet added ourself to the peer list,
    // so no check is needed
    for (int i = 0; i < (int) group_set.size(); i++) {
        peer =  group_set[i];
        glob_log.log(3, "Notifying peer %s.\n", cli_to_str(peer));

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        // Construct the outgoing message
        construct_message(PEER, &out_msg, my_info, sizeof(struct client_info));

        // Send!
        if (send_message(sock, &out_msg) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Get reply
        if (read_message(sock, &in_msg) < 0)
            error(-1, errno, "Could not receive peer response");
        if (in_msg.type != READY)
            error(-1, errno, "received bad peer response message type");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");
    }

    return true;
}

bool Broadcast_Channel::send_peer_list(int sock, struct client_info *target) {
    struct message out_msg;
    struct client_info *peer;

    // First, send info about the way we see the target
    target->id = get_unused_id();

    // Construct the outgoing message
    construct_message(PEER, &out_msg, target, sizeof(struct client_info));

    // Send!
    if (send_message(sock, &out_msg) != sizeof(out_msg))
        error(-1, errno, "Could not send peer notification");

    // Next, send the current list of peers
    for (int i = 0; i < (int) group_set.size(); i++) {
        peer =  group_set[i];
        glob_log.log(3, "Sending info for peer %s.\n", cli_to_str(peer));

        // Construct the outgoing message
        construct_message(PEER, &out_msg, peer, sizeof(struct client_info));

        // Send!
        if (send_message(sock, &out_msg) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");
    }

    return true;
}

void Broadcast_Channel::add_peer(struct message *in_msg) {
    if (in_msg->type != PEER)
        error(-1, EIO, "Tried to add peer from non-peer message");

    struct client_info *peer_info;
    peer_info = (struct client_info *) malloc(sizeof(client_info));
    memcpy(peer_info, &in_msg->data, sizeof(struct client_info));
    group_set.push_back(peer_info);

    glob_log.log(3, "received peer request! Peer %s\n", cli_to_str(peer_info));
}

unsigned int Broadcast_Channel::get_unused_id() {
    unsigned int max_used = 0;
    for (int i = 0; i < (int) group_set.size(); i++)
        if (group_set[i]->id > max_used) max_used = group_set[i]->id;
    return max_used + 1;
}

void *Broadcast_Channel::start_server(void *args) {
    Broadcast_Channel *bc = (Broadcast_Channel *) args;
    bc->accept_connections();
    return NULL;
}

void Broadcast_Channel::accept_connections() {
    int client_sock;
    struct message in_msg, out_msg;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    struct client_info *peer_info;
    //time vars
    unsigned int confirm_msgid;
    msg_t confirm_type;
    struct timespec cur_time;
    int64_t time_dif;
    std::pair< int, struct timespec > time_info;
    //broadcast completion time logger
    std::string t_log_f_name("_time_msg_size.txt");
    t_log_f_name = my_info->name + t_log_f_name;
    logger t_log = logger(1, t_log_f_name.c_str());


    //Start listening on the chunk receiver socket
    listen(server_sock, 10);

    bool running = true;
    while (running) {
        glob_log.log(3, "Server waiting...\n");

        // Wait for a connection
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
            error(-1, errno, "Could not accept client.");
        glob_log.log(3, "accepted client!\n");

        // Handle the request
        if (read_message(client_sock, &in_msg) < 0)
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

                if (send_message(client_sock, &out_msg) != sizeof(out_msg))
                    error(-1, errno, "Could not send ready reply");
                break;

            case QUIT:
                peer_info = (struct client_info *) in_msg.data;
                glob_log.log(3, "received quit message from %s\n", cli_to_str(peer_info));
                if (peer_info->id == my_info->id) {
                    // Shutdown message from the CLI thread
                    glob_log.log(3, "Shutting down receive thread\n");
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
                    glob_log.log(3, "received quit notice from an unknown peer: %s",
                            cli_to_str(peer_info));
                }
                break;

            case CONFIRM:
                memcpy(&confirm_msgid, in_msg.data, sizeof(confirm_msgid));
                memcpy(&confirm_type, in_msg.data + sizeof(confirm_msgid), sizeof(confirm_type));

                if (-1 == clock_gettime(clk, &cur_time))
                    error(-1, errno, "Unable to get current time");
                time_info = start_times[confirm_msgid];
                time_info.first -= 1;
                if(time_info.first == 0){
                    time_dif = cur_time.tv_sec - time_info.second.tv_sec;
                    time_dif *= 1000000000;//convert to ns
                    time_dif += (cur_time.tv_nsec - time_info.second.tv_nsec);
                    time_dif /= 1000;
                    glob_log.log(2, "Received final confirmation message for msg %u\n", confirm_msgid);
                    glob_log.log(2, "Took %lld microseconds!\n",
                        (long long)time_dif);

                    t_log.log(1, "%s %lld %zu\n", msg_t_to_str(confirm_type),
                        (long long)time_dif, group_set.size());
                }
                start_times[confirm_msgid] = time_info;
                break;

            case CLIENT_SERVER:
            case TRAD:
            case COOP:
            case LT:
                handle_chunk(client_sock, &in_msg);

                break;
            case RAPTOR:
                // Not implemented
                glob_log.log(3, "Oh Noes! We got a message we don't know what to do with...\n");

            default:
                break;
        }

        // Close socket
        if (close(client_sock) != 0)
            error(-1, errno, "Error closing peer socket");

    }
}

void Broadcast_Channel::handle_chunk(int client_sock, struct message *in_msg) {
    std::list< struct message * > msg_list;
    struct message *msg = NULL;
    decoder *msg_dec = NULL;
    bool deliver_msg = false;
    unsigned long dec_id = 0;

    if (in_msg->cli_id == my_info->id)
        return;  // It's just a bump of one of our own messages

    // Keep track of the messages we've gotten, so we can forward them along
    msg = (struct message *)malloc(sizeof(struct message));
    memcpy(msg, in_msg, sizeof(struct message));
    msg_list.push_back(msg);

    glob_log.log(3, "received %s message\n", msg_t_to_str(in_msg->type));

    // Get a decoder for this message
    dec_id = (((unsigned long)in_msg->cli_id) << 32) | (unsigned long)in_msg->msg_id;
    if(decoders.find(dec_id) == decoders.end()){
        msg_dec = get_decoder(in_msg->type);
        decoders[dec_id] = msg_dec;
    }
    msg_dec = decoders[dec_id];
    deliver_msg = !msg_dec->is_ready();

    // Add the chunk we just got
    msg_dec->add_chunk(in_msg->data, in_msg->data_len, in_msg->chunk_id);
    dump_buf(3, in_msg->data, in_msg->data_len);

    // Keep reading chunks and adding them until the bcast is done
    while (in_msg->data_len != 0) {
        if (read_message(client_sock, in_msg) < 0)
            error(-1, errno, "Could not receive client message");

        msg = (struct message *)malloc(sizeof(struct message));
        memcpy(msg, in_msg, sizeof(struct message));
        msg_list.push_back(msg);

        glob_log.log(2, "Recieved chunk %u of msg %u from peer %u\n",
                in_msg->chunk_id, in_msg->msg_id, in_msg->cli_id);

        msg_dec->add_chunk(in_msg->data, in_msg->data_len, in_msg->chunk_id);
        dump_buf(3, in_msg->data, in_msg->data_len);
    }

    // Forward the message on to the other peers
    if (msg_dec->should_forward() && in_msg->ttl > 0) {
        forward(msg_list);
    }

    // Print the message and clean up
    if(msg_dec->is_ready()){
        if(deliver_msg){
            // Deliver the message to the application
            listener->receive(msg_dec->get_message(), msg_dec->get_len());

            // Construct the confirm message
            struct message finish_msg;
            memset(&finish_msg, 0, sizeof(finish_msg));
            // Set the message data (msgid, algo)
            unsigned char finish_msg_data[sizeof(unsigned) + sizeof(msg_t)];
            memcpy(finish_msg_data, &(in_msg->msg_id), sizeof(unsigned));
            memcpy(finish_msg_data + sizeof(unsigned), &(in_msg->type), sizeof(msg_t));
            construct_message(CONFIRM, &finish_msg, &finish_msg_data, sizeof(finish_msg_data));
            // Get the original sender's ip info
            int index;
            for (index = 0; index < (int) group_set.size(); index++) {
                if (group_set[index]->id == in_msg->cli_id)
                    break;
            }
            if (index == (int) group_set.size())
                error(-1, 0, "Could not find original broadcaster info struct\n");
            struct client_info *peer = group_set[index];
            // Open a socket to the original broadcaster
            int confirm_sock = make_socket();
            if (connect(confirm_sock, (struct sockaddr *) &peer->ip, sizeof(peer->ip)) < 0)
                error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));
            // Send the confirm message
            if (send_message(confirm_sock, &finish_msg) != sizeof(finish_msg))
                error(-1, errno, "Could not send ready CONFIRM message");
            // Close the socket
            close(confirm_sock);
        }
        if(msg_dec->is_finished())
            decoders.erase(dec_id);
    }
}

bool Broadcast_Channel::join(std::string hostname, int port){
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

void Broadcast_Channel::quit() {
    int sock;
    struct client_info *peer;
    struct message out_msg;
    glob_log.log(3, "Putting in 2 weeks' notice\n");

    // Notify peers that we're quitting
    // Note: this will include our listener thread - this is good, because
    // it provides an easy way to tell it to shut down.
    for (int i = 0; i < (int)group_set.size(); i++) {
        peer =  group_set[i];
        glob_log.log(3, "Notifying peer %s.\n", cli_to_str(peer));

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        // Construct the outgoing message
        construct_message(QUIT, &out_msg, my_info, sizeof(struct client_info));

        // Send
        if (send_message(sock, &out_msg) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");
    }

    pthread_join(receiver_thread, NULL);
}

void Broadcast_Channel::broadcast(msg_t algo, unsigned char *buf, size_t buf_len){
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

    // Get the starting time of this broadcast
    struct timespec start_time;
    if(-1 == clock_gettime(clk, &start_time))
        error(-1, errno, "Unable to get current time");
    start_times[msg_counter] = std::make_pair((int)group_set.size()-1, start_time);

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
            glob_log.log(2, "Sending chunk %u of msg %lu to peer %u\n",
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

            dump_buf(3, out_msg.data, out_msg.data_len);

            // Send the message
            if (send_message(sock, &out_msg) != sizeof(out_msg))
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
        if (send_message(sock, &out_msg) != sizeof(out_msg))
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
void Broadcast_Channel::forward(std::list< struct message * > msg_list) {
    int sock;
    struct client_info *peer;
    for (int i = 0; i < (int)group_set.size(); i++) {
        peer =  group_set[i];

        //Don't broadcast to yourself or the origin
        if(peer->id == my_info->id || peer->id == msg_list.front()->cli_id) continue;

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        pthread_t forward_thread;
        struct forward_event *fe = new struct forward_event;
        fe->sock = sock;
        fe->msg_list = msg_list;
        fe->peer_id = peer->id;
        fe->this_ptr = this;
        if(i == (int)group_set.size() - 1) fe->should_delete = true;
        else fe->should_delete = false;

        pthread_create( &forward_thread, NULL, do_forward, (void *)fe);
    }
}

void *Broadcast_Channel::do_forward(void *arg){
    struct forward_event *fe = (struct forward_event *)arg;
    struct message *out_msg = NULL;

    // Send all messages
    for (std::list< struct message * >::iterator it = fe->msg_list.begin();
            it != fe->msg_list.end(); it++) {
        out_msg = *it;
        if (out_msg->data_len != 0) {
            glob_log.log(2, "Forwarding chunk %u of msg %u from peer %u to peer %u\n",
                    out_msg->chunk_id, out_msg->msg_id,
                    out_msg->cli_id, fe->peer_id);
        } else {
            glob_log.log(2, "Forwarding terminal chunk of msg %u "
                    "from peer %u to peer %u\n",
                    out_msg->msg_id,
                    out_msg->cli_id, fe->peer_id);
        }

        out_msg->ttl = 0;  // We don't want people to rebroadcast rebroadcasts
        if (fe->this_ptr->send_message(fe->sock, out_msg) != sizeof(struct message))
            error(-1, errno, "Could not forward transmission");
    }

    // Close socket
    if (close(fe->sock) != 0)
        error(-1, errno, "Error closing peer socket");

    if(fe->should_delete){
        for (std::list< struct message * >::iterator it = fe->msg_list.begin();
                it != fe->msg_list.end(); it++) {
            free(*it);
        }
    }
    free(fe);

    return NULL;
}
