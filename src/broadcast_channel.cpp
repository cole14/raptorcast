#include <arpa/inet.h>
#include <error.h>
#include <errno.h>
#include <memory>
#include <netdb.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <set>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <sstream>

#include "broadcast_channel.h"

#include "logger.h"

//gethostbyname error num
extern int h_errno;
extern int errno;

// Local Utility functions
static const char * msg_t_to_str(msg_t type) {
    switch (type) {
        case JOIN:
            return "JOIN";
        case PEER:
            return "PEER";
        case PING:
            return "PING";
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

static const char *cli_to_str(struct client_info *cli) {
    const int max_size = 256;
    static char str[max_size];
    snprintf(str, max_size, "%u:%s at %s:%d",
            cli->id,
            cli->name,
            inet_ntoa(cli->ip.sin_addr),
            ntohs(cli->ip.sin_port));
    return str;
}

client_info::client_info(std::string n)
:ip()
,id(0)
{
    //Sanity check the name length
    size_t len = n.length()+1;
    if (len > MAX_NAME_LEN)
        error(-1, EINVAL, "Chosen name (%s) is too long (max %d)", n.c_str(), MAX_NAME_LEN);

    //Copy name contents
    strcpy(name, n.c_str());
    //Zero-out the rest of the name
    memset(name+len, 0, MAX_NAME_LEN-len);
}

// Constructors/Destructors
Broadcast_Channel::~Broadcast_Channel(void)
{
    // Quit the broadcast group (NOP if not connected)
    try{
        quit();

        //Delete the group_set
        // Note: 'my_info' is added to the group_set, so deleting the group_set deletes it as well.
        for(std::vector< struct client_info * >::iterator it = group_set.begin(); it != group_set.end(); it++){
            free(*it);
        }
    }catch(...){
        fprintf(stderr, "Caught unexpected exception within ~Broadcast_Channel()\n");
    }
}

//Default constructor - initialize the port member
Broadcast_Channel::Broadcast_Channel(std::string name, std::string port, Channel_Listener *lstnr)
:my_info(NULL)
,group_set()
,decoders()
,receiver_thread()
,server_sock(-1)
,connected(false)
,listener(lstnr)
,msg_counter(0)
,down_mode(false)
,start_times()
{
    //Allocate the client_info struct
    my_info = new struct client_info(name);

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

//Look up the client_info based on client id
//  Returns NULL if not found
client_info *Broadcast_Channel::get_peer_by_id(unsigned int id) {
    for (size_t i = 0; i < group_set.size(); i++)
        if (group_set[i]->id == id)
            return group_set[i];
    return NULL;
}

//Flip the value of debug_mode
bool Broadcast_Channel::toggle_node_down(){
    return (down_mode = !down_mode);
}

//Set up and return a new unconnected socket with the proper options
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

void Broadcast_Channel::print_msgs(int indent) {
    //Preconstruct the indentation string
    std::stringstream idnt;
    for(int i = 0; i < indent; i++)
        idnt << "    ";

    //Print out the completed message info
    //    Peer: # Message: #
    glob_log.log(1, "%s\n", "Completed Messages:");
    for(std::set< uint64_t >::iterator it = finished_messages.begin(); it != finished_messages.end(); it++){
        uint64_t dec_id = *it;
        uint64_t cli_id = (dec_id & 0xFFFFFFFF00000000LL) >> 32;
        uint64_t msg_id = dec_id & 0xFFFFFFFFLL;

        glob_log.log(1, "%sPeer: %u Message: %u\n", idnt.str().c_str(), (unsigned)cli_id, (unsigned)msg_id);
    }

    //Print out the outstanding message info
    //    Peer: # Message: #
    //        Chunks: # # ... #
    //        Blocks: # # ... #
    glob_log.log(1, "%s\n", "Outstanding Messages:");
    std::vector< unsigned > msg_info;
    for(std::map< uint64_t, Incoming_Message * >::iterator it = decoders.begin(); it != decoders.end(); it++){
        uint64_t dec_id = it->first;
        uint64_t cli_id = (dec_id & 0xFFFFFFFF00000000LL) >> 32;
        uint64_t msg_id = dec_id & 0xFFFFFFFFLL;

        glob_log.log(1, "%sPeer: %u Message: %u:\n", idnt.str().c_str(), (unsigned)cli_id, (unsigned)msg_id);

        glob_log.log(1, "%s%sChunks: ", idnt.str().c_str(), "    ");
        it->second->fill_chunk_list(&msg_info);
        glob_log.pretty_print_id_list(1, msg_info);

        glob_log.log(1, "%s%sBlocks: ", idnt.str().c_str(), "    ");
        it->second->fill_block_list(&msg_info);
        glob_log.pretty_print_id_list(1, msg_info);
    }
}

/* group_set management functions */
void Broadcast_Channel::print_peers(int indent) {
    struct client_info *peer;
    for (unsigned int i = 0; i < group_set.size(); i++) {
        peer = group_set[i];
        for(int i = 0; i < indent; i++) glob_log.log(1, "%s", "    ");
        glob_log.log(1, "Peer %s\n", cli_to_str(peer));
    }
}

/*
 *   ____              _       _
 *  |  _ \            | |     | |
 *  | |_) | ___   ___ | |_ ___| |_ _ __ __ _ _ __
 *  |  _ < / _ \ / _ \| __/ __| __| '__/ _` | '_ \
 *  | |_) | (_) | (_) | |_\__ \ |_| | | (_| | |_) |
 *  |____/ \___/ \___/ \__|___/\__|_|  \__,_| .__/
 *                                          | |
 *                                          |_|
 */

bool Broadcast_Channel::join(std::string hostname, int port){
    if (hostname.empty()) {
        // Start new broadcast group
        my_info->id = 1;

    } else {
        // Join an existing broadcast group
        get_peer_list(hostname, port);

        // Notify the group_set that we are ready to go
        notify_peers();
    }

    // Add myself to the peer list
    group_set.push_back(my_info);

    pthread_create( &receiver_thread, NULL, start_server, (void *)this);

    return true;
}

/*
 * Request the group set from a known host (the "strap")
 * Sends a JOIN message to hostname:port, containing information about this
 * host.  The strap should respond with a PEER message containing information
 * about us complete with ID, and then 1 or more PEER messages containing info
 * on the other peers in the network.
 */
void Broadcast_Channel::get_peer_list(std::string hostname, int port) {
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

    //Extract the client id from the strap's response
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
}

/*
 * Send a PEER message containing information about us to everyone on the peer
 * list.  They will respond with a READY, indicating that they have added us to
 * their peer list, and are OK to receive messages from us.
 */
void Broadcast_Channel::notify_peers() {
    int sock;
    struct message in_msg, out_msg;
    struct client_info *peer;

    // The group_set should at least have ourselves to notify.
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
        if (connect(sock, (struct sockaddr *) &peer->ip, sizeof(peer->ip)) < 0)
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
}


void Broadcast_Channel::add_peer(struct message *in_msg) {
    //Sanity check
    if (in_msg->type != PEER)
        error(-1, EIO, "Tried to add peer from non-peer message");

    //Allocate the new client_info
    struct client_info *peer_info = (struct client_info *) malloc(sizeof(client_info));
    memcpy(peer_info, &in_msg->data, sizeof(struct client_info));

    //Add it to the group_set
    group_set.push_back(peer_info);

    //log
    glob_log.log(3, "received peer request! Peer %s\n", cli_to_str(peer_info));
}


// Note: this is NOT thread-safe.  Don't quit two peers at once.
void Broadcast_Channel::remove_peer(struct client_info *peer_info) {
    int index;
    for (index = 0; index < (int) group_set.size(); index++) {
        if (group_set[index]->id == peer_info->id)
            break;
    }

    //Remove and free the client from the group_set
    if (index < (int) group_set.size()) {
        struct client_info *quitter = group_set[index];
        group_set.erase(group_set.begin() + index);
        free(quitter);
    } else {
        glob_log.log(3, "received quit notice from an unknown peer: %s",
                cli_to_str(peer_info));
    }
}

//Return the next available client_id
unsigned int Broadcast_Channel::get_unused_id() {
    unsigned int max_used = 0;
    for (int i = 0; i < (int) group_set.size(); i++)
        if (group_set[i]->id > max_used) max_used = group_set[i]->id;
    return max_used + 1;
}

//Wrapper function for use in spawning the server thread
void *Broadcast_Channel::start_server(void *args) {
    ((Broadcast_Channel *)args)->accept_connections();
    return NULL;
}


/*   _____
 *  / ____|
 * | (___   ___ _ ____   _____ _ __
 *  \___ \ / _ \ '__\ \ / / _ \ '__|
 *  ____) |  __/ |   \ V /  __/ |
 * |_____/ \___|_|    \_/ \___|_|
 */

//Server function to handle incoming messages
void Broadcast_Channel::accept_connections() {
    int client_sock;
    struct message in_msg;
    struct sockaddr_in client_addr = sockaddr_in();
    socklen_t client_len = socklen_t();
    //time vars
    unsigned int confirm_msgid;
    msg_t confirm_type;
    std::chrono::duration< double, std::ratio< 1, 1000000 > > time_dif;//period=microseconds
    std::pair< int, std::chrono::system_clock::time_point > time_info;
    //broadcast completion time logger
    std::string t_log_f_name("_time_msg_size.txt");
    t_log_f_name = my_info->name + t_log_f_name;
    logger t_log = logger(1, t_log_f_name.c_str());


    //Start listening on the chunk receiver socket
    listen(server_sock, 10);

    connected = true;
    while (connected) {
        glob_log.log(4, "Server waiting...\n");

        // Wait for a connection
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0)
            error(-1, errno, "Could not accept client.");
        glob_log.log(3, "accepted client!\n");

        // Handle the request
        if (read_message(client_sock, &in_msg) < 0)
            error(-1, errno, "Could not receive client message");

        glob_log.log(2, "Handling %s message\n", msg_t_to_str(in_msg.type));
        switch (in_msg.type) {
            case JOIN :
                handle_join(client_sock, &in_msg);
                break;

            case PEER:
                handle_peer(client_sock, &in_msg);
                break;

            case PING:
                handle_ping(client_sock, &in_msg);
                break;

            case QUIT:
                handle_quit(&in_msg);
                break;

            case CONFIRM:
                memcpy(&confirm_msgid, in_msg.data, sizeof(confirm_msgid));
                memcpy(&confirm_type, in_msg.data + sizeof(confirm_msgid), sizeof(confirm_type));

                time_info = start_times[confirm_msgid];
                time_info.first -= 1;
                if(time_info.first == 0){
                    time_dif = std::chrono::system_clock::now() - time_info.second;
                    glob_log.log(2, "Received final confirmation message for msg %u\n", confirm_msgid);
                    glob_log.log(2, "Took %.lf microseconds!\n", time_dif.count());

                    t_log.log(1, "%s %.lf %zu\n", msg_t_to_str(confirm_type), time_dif.count(), group_set.size());
                }
                start_times[confirm_msgid] = time_info;
                break;

            case CLIENT_SERVER:
            case TRAD:
            case COOP:
            case LT:
                if (!down_mode)
                    handle_chunk(client_sock, &in_msg);
                else
                    glob_log.log(2, "Ignoring message because node is down\n");

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

    // Close down the server
    close(server_sock);
}

/*
 * Send PEER messages containing the information about each client in the group_set.
 * This is part of the bootstrap process and includes/defines the target's
 * client id.
 */
void Broadcast_Channel::handle_join(int sock, struct message *in_msg) {
    struct client_info *target = (struct client_info *)&in_msg->data;
    struct message out_msg;
    struct client_info *peer;

    // First, define the target's client id
    target->id = get_unused_id();

    // Construct the outgoing message out of the target's info
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
}

/*
 * Handlers for different message types
 */
void Broadcast_Channel::handle_ping(int client_sock, struct message *in_msg) {
    struct message out_msg;
    // Construct the outgoing message
    construct_message(PING, &out_msg, my_info, sizeof(struct client_info));

    // Send
    if (send_message(client_sock, &out_msg) != sizeof(out_msg))
        error(-1, EIO, "Trouble returning PING");
}

void Broadcast_Channel::handle_peer(int client_sock, struct message *in_msg) {
    struct message out_msg;

    // Add the new peer to the group
    add_peer(in_msg);

    // Let them know we're ready to go
    construct_message(READY, &out_msg, NULL, 0);
    if (send_message(client_sock, &out_msg) != sizeof(out_msg))
        error(-1, errno, "Could not send ready reply");
}

void Broadcast_Channel::handle_quit(struct message *in_msg) {
    //Sanity check
    if (in_msg->type != QUIT) {
        error(-1, EIO, "Expected QUIT message, but got %s",
                msg_t_to_str(in_msg->type));
    }

    struct client_info *peer_info = (struct client_info *) in_msg->data;
    glob_log.log(3, "received quit message from %s\n", cli_to_str(peer_info));
    if (peer_info->id == my_info->id) {
        // Shutdown message from the CLI thread
        glob_log.log(3, "Shutting down receive thread\n");
        connected = false;
        return;
    }

    return remove_peer(peer_info);
}

void Broadcast_Channel::handle_chunk(int client_sock, struct message *in_msg) {
    std::list< std::shared_ptr<struct message> > msg_list;
    struct message *msg = NULL;
    Incoming_Message *decoder = NULL;
    uint64_t dec_id = 0;

    if (in_msg->cli_id == my_info->id)
        return;  // It's just a bump of one of our own messages

    // Get a decoder for this message
    dec_id = (((uint64_t)in_msg->cli_id) << 32) | (uint64_t)in_msg->msg_id;
    if (finished_messages.find(dec_id) != finished_messages.end()) {
        /* TODO (Dan 2/25/13)
         * Make sure we forward messages for finished decoders.
         */
        return;
    }

    if(decoders.find(dec_id) == decoders.end())
        decoders[dec_id] = new Incoming_Message(in_msg->type);
    decoder = decoders[dec_id];

    // Keep reading chunks and adding them until the bcast is done
    do {
        msg = new struct message();
        memcpy(msg, in_msg, sizeof(struct message));
        msg_list.push_back(std::shared_ptr<struct message>(msg));

        glob_log.log(2, "Recieved chunk %u of msg %u from peer %u\n",
                in_msg->chunk_id, in_msg->msg_id, in_msg->cli_id);
        glob_log.dump_buf(3, in_msg->data, in_msg->data_len);

        decoder->add_chunk(in_msg->data, in_msg->data_len, in_msg->chunk_id);

        // Read the next chunk
        if (read_message(client_sock, in_msg) < 0)
            error(-1, errno, "Could not receive client message");
    } while (in_msg->chunk_id != MSG_END);

    // Add the terminator message to the list (but not the decoder)
    msg = new struct message();
    memcpy(msg, in_msg, sizeof(struct message));
    msg_list.push_back(std::shared_ptr<struct message>(msg));

    // Forward the message on to the other peers
    if (decoder->should_forward() && in_msg->ttl > 0) {
        forward(msg_list);
    }

    // Print the message and clean up
    if(decoder->is_ready()){
        // Deliver the message to the application
        listener->receive(decoder->get_message(), decoder->get_len());

        // Let the sender know we got it
        confirm_message(in_msg);

        glob_log.log(2, "Erasing decoder for message %u\n", in_msg->msg_id);
        finished_messages.insert(dec_id);
        decoders.erase(dec_id);
        delete decoder;
    }
}

void Broadcast_Channel::confirm_message(struct message *in_msg) {
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

/*
 * Forward a list of messages to the rest of the peer group
 */
void Broadcast_Channel::forward(std::list< std::shared_ptr< struct message > > msg_list) {
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

        pthread_create( &forward_thread, NULL, do_forward, (void *)fe);
    }
}

void *Broadcast_Channel::do_forward(void *arg){
    struct forward_event *fe = (struct forward_event *)arg;
    struct message *out_msg = NULL;

    // Send all messages
    for (std::list< std::shared_ptr< struct message > >::iterator it = fe->msg_list.begin();
            it != fe->msg_list.end(); it++) {
        out_msg = (*it).get();
        if (out_msg->data_len != 0) {
            glob_log.log(2, "Forwarding chunk %d of msg %u from peer %u to peer %u\n",
                    out_msg->chunk_id, out_msg->msg_id,
                    out_msg->cli_id, fe->peer_id);
        } else {
            glob_log.log(2, "Forwarding terminal chunk of msg %u "
                    "from peer %u to peer %u\n",
                    out_msg->msg_id,
                    out_msg->cli_id, fe->peer_id);
        }

        out_msg->ttl = 0;  // We don't want people to rebroadcast rebroadcasts
        if (fe->this_ptr->send_message(fe->sock, out_msg) != sizeof(struct message)) {
            /* XXX (Dan 2/27/13):
             * This is the location for issue #26
             */
            glob_log.log(1, "Had trouble forwarding a transmission: %s\n", strerror(errno));
            fe->this_ptr->ping(-1);
            break;
            /*
            error(-1, errno, "Could not forward transmission to peer %u",
                    out_msg->cli_id);
                    */
        }
    }

    // Close socket
    if (close(fe->sock) != 0)
        error(-1, errno, "Error closing peer socket");

    delete fe;

    return NULL;
}

void Broadcast_Channel::broadcast(msg_t algo, unsigned char *data, size_t data_len){
    int sock;
    struct client_info *peer;

    Outgoing_Message *msg_handler;
    msg_handler = new Outgoing_Message(algo, data, data_len, group_set.size()-1, PACKET_LEN);

    // Increment the message id counter
    msg_counter++;

    // Get the starting time of this broadcast
    start_times[msg_counter] = std::make_pair((int)group_set.size()-1, std::chrono::system_clock::now());

    // Ensures that all peer requests are contiguous ints starting at 0 and ending at
    //   group_set.size()-2 (because we don't broadcast to ourselves.
    unsigned peer_count = 0;

    // Continually generate chunks until the decoder is out of chunks
    unsigned char *chunk = NULL;
    unsigned chunk_id = 0;
    struct message out_msg;
    for (unsigned i = 0; i < group_set.size(); i++, peer_count++) {
        peer = group_set[i];

        // Don't broadcast to yourself
        if(peer->id == my_info->id) {
            peer_count--;
            continue;
        }

        // Setup the socket
        sock = make_socket();

        // Open socket
        if (connect(sock, (struct sockaddr *) &peer->ip,
                    sizeof(peer->ip)) < 0)
            error(-1, errno, "Could not connect to peer %s", cli_to_str(peer));

        // Get chunks to send to peer
        std::vector< std::pair<int, unsigned char *> > *chunks;
        chunks = msg_handler->get_chunks(peer_count);

        for (int c = 0; c < (int)chunks->size(); c++) {
            // Extract the chunk data
            chunk_id = (*chunks)[c].first;
            chunk = (*chunks)[c].second;

            // Build the message around the chunk
            out_msg.type = algo;
            out_msg.cli_id = my_info->id;
            out_msg.msg_id = msg_counter;       // We don't want a new msgid for each chunk
            out_msg.chunk_id = chunk_id;
            out_msg.ttl = 1;
            out_msg.data_len = PACKET_LEN;      // The data is padded, so this is always true
            memset(&out_msg.data, 0, PACKET_LEN);
            memcpy(&(out_msg.data), chunk, PACKET_LEN);

            glob_log.dump_buf(3, out_msg.data, out_msg.data_len);

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
        out_msg.chunk_id = MSG_END;
        out_msg.ttl = 1;
        out_msg.data_len = 0;
        memset(&out_msg.data, 0, PACKET_LEN);

        // Send the message
        if (send_message(sock, &out_msg) != sizeof(out_msg))
            error(-1, errno, "Could not send peer notification");

        // Close socket
        if (close(sock) != 0)
            error(-1, errno, "Error closing peer socket");

        delete chunks;
    }
    delete msg_handler;
}

/*
 * Verify connecivity with a specified peer, and purge them from
 * the list if they're unavailable.
 * Negative inputs -> ping all peers
 * Returns the number of peers that were removed from the list
 */
unsigned Broadcast_Channel::ping(int in_peer_id) {
    unsigned index;
    // Deal with a ping-all recursively
    if (in_peer_id < 0) {
        unsigned removed = 0;
        for (index = 0; index < group_set.size(); index++) {
            unsigned result = ping(group_set[index]->id);
            index -= result;
            removed += result;
        }
        return removed;
    }

    // Find the target peer
    unsigned peer_id = (unsigned) in_peer_id;
    for (index = 0; index < group_set.size(); index++) {
        if (group_set[index]->id == peer_id)
            break;
    }
    if (index >= group_set.size()) {
        // Tried to ping a peer that doesn't exist! LOL!
        return 0;
    }

    struct client_info *peer;
    struct message out_msg, in_msg;
    int sock;

    peer = group_set[index];
    glob_log.log(1, "Pinging peer %s.\n", cli_to_str(peer));

    // Setup the socket
    sock = make_socket();

    // Open socket
    if (connect(sock, (struct sockaddr *) &peer->ip, sizeof(peer->ip)) < 0) {
        glob_log.log(1, "Could not connect to peer: %s\n", strerror(errno));
        remove_peer(peer);
        return 1;
    }

    // Construct the outgoing message
    construct_message(PING, &out_msg, my_info, sizeof(struct client_info));

    // Send
    if (send_message(sock, &out_msg) != sizeof(out_msg)) {
        glob_log.log(1, "Could not ping peer: %s\n", strerror(errno));
        remove_peer(peer);
        return 1;
    }

    // Get reply
    if (read_message(sock, &in_msg) < 0) {
        glob_log.log(1, "Could not receive peer response: %s\n", strerror(errno));
        remove_peer(peer);
        return 1;
    }
    if (in_msg.type != PING) {
        glob_log.log(1, "received bad peer response message type: %s\n", strerror(errno));
        remove_peer(peer);
        return 1;
    }

    // Close socket - if this fails, we've probably got bigger problems than
    // a bad peer, so we just barf.
    if (close(sock) != 0)
        error(-1, errno, "Error closing peer socket");

    return 0;
}

void Broadcast_Channel::quit() {
    int sock;
    struct client_info *peer;
    struct message out_msg;

    //Only try to quit if we're connected
    if(!connected)
        return;

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
