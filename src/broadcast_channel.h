#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <netinet/ip.h>
#include <stddef.h>
#include <time.h>

#include <list>
#include <map>
#include <utility>
#include <string>
#include <vector>

#include "channel_listener.h"
#include "dec/decoder.h"
#include "enc/encoder.h"

#include "message_types.h"

struct client_info {
    struct sockaddr_in ip;
    unsigned int id;
    char name[MAX_NAME_LEN];
};

class Broadcast_Channel {
    public:
        // Constructor
        Broadcast_Channel(std::string name, std::string port, Channel_Listener *lstnr);

        // Default destructor
        virtual ~Broadcast_Channel(void);

        // Connect to an existing broadcast group
        bool join(std::string hostname, int port);

        // Broadcast a message over this channel
        void broadcast(msg_t algo, unsigned char *buf, size_t buf_len);

        // Print the list of known peers at indentation level 'indent' to stdout
        void print_peers(int indent = 0);

        // Notify peers that we're quitting, clean up connections, etc.
        void quit();

        // Toggle the debug mode for this broadcast channel.
        bool toggle_debug_mode();

    private:
        // The client info for this broadcast_channel
        struct client_info *my_info;
        // The client info for everyone in the broadcast group
        std::vector< struct client_info * > group_set;
        // Association between message_ids and their active decoders
        std::map< unsigned long, Incoming_Message * > decoders;
        // The chunk receiver thread
        pthread_t receiver_thread;
        // The chunk receiver socket
        int server_sock;
        // The application listening on this channel
        Channel_Listener *listener;
        // The monotonically increasing unique message id counter for this sender
        unsigned long msg_counter;
        // Flag to specify whether to artifically crash at specified points
        bool debug_mode;
        // Timers for message broadcast completion timing.
        std::map< unsigned int, std::pair< int, struct timespec > > start_times;
        clockid_t clk;

        // Contact a known host and get a list of all peers
        bool get_peer_list(std::string hostname, int port);
        // Send a list of all peers to a new member
        bool send_peer_list(int client_sock, struct client_info *target);
        // Tell the list of peers that we exist
        bool notify_peers();
        // Add a peer to the list
        void add_peer(struct message *);

        // Get an id that is not currently in use within the peer set
        // Note that this is terrible and presents all sorts of race conditions
        unsigned int get_unused_id();
        // Lookup a client, by id
        client_info *get_peer_by_id(unsigned int id);

        // Wrapper func for start_server for the purpose of threadding
        static void *start_server(void *);
        //
        static void *do_forward(void *);
        // Wait for and handle network requests from other peers
        void accept_connections();
        // Deal with a chunk of a message
        void handle_chunk(int client_sock, struct message *in_msg);
        // Send a list of chunks to every peer in the group
        void forward(std::list< struct message * > msg_list);

        // Put together a message containing the given data
        void construct_message(msg_t type, struct message *dest, const void *src, size_t n);

        // Create a tcp socket
        int make_socket();
        // Read a message from a tcp socket into 'msg'
        ssize_t read_message(int sock, struct message *msg);
        // Send a message over a tcp socket
        ssize_t send_message(int sock, struct message *msg);
};

struct forward_event {
    int sock;
    std::list< struct message * > msg_list;
    unsigned int peer_id;
    Broadcast_Channel *this_ptr;
    bool should_delete;
};

#endif /* __BROADCAST_CHANNEL_H */
