#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <netinet/ip.h>
#include <stddef.h>

#include <chrono>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "channel_listener.h"
#include "dec/decoder.h"
#include "enc/encoder.h"

#include "message_types.h"

struct client_info {
    client_info(std::string name);
    struct sockaddr_in ip;
    unsigned id;
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
        // 'buf' is owned by the caller.
        void broadcast(msg_t algo, unsigned char *buf, size_t buf_len);

        // Print the list of known peers at indentation level 'indent' to the glob_log
        void print_peers(int indent = 0);

        // Print the message history at indentation level 'indent' to the glob_log
        void print_msgs(int indent = 0);

        // Notify peers that we're quitting, clean up connections, etc.
        void quit();

        // Ping somebody (-1 for everybody, otherwise by client id)
        unsigned ping (int peer_id);

        // Simulate a node going down
        bool toggle_node_down();

    private:
    /* Private Members */
        // The client info for this broadcast_channel
        struct client_info *my_info;
        // The client info for everyone in the broadcast group
        std::vector< struct client_info * > group_set;
        // Association between message_ids and their active decoders
        std::map< uint64_t, Incoming_Message * > decoders;
        std::set< uint64_t > finished_messages;
        // The chunk receiver thread
        pthread_t receiver_thread;
        // The chunk receiver socket
        int server_sock;
        // True if currently connected to a group, false otherwise
        bool connected;
        // The application listening on this channel
        Channel_Listener *listener;
        // The monotonically increasing unique message id counter for this sender
        unsigned long msg_counter;
        // When true, will act dead (receiving messages but not forwarding)
        bool down_mode;

        // Timers for message broadcast completion timing.
        std::map< unsigned int, std::pair< int, std::chrono::system_clock::time_point > > start_times;

    /* Private Functions */
        /*
         * Utility functions
         */
        // Get an id that is not currently in use within the peer set
        // Note that this is terrible and presents all sorts of race conditions
        unsigned int get_unused_id();
        // Lookup a client, by id
        client_info *get_peer_by_id(unsigned int id);

        /*
         * Basic networking, bootstrap and maintinance stuff
         */
        // Contact a known host and get a list of all peers
        void get_peer_list(std::string hostname, int port);
        // Tell the list of peers that we exist
        void notify_peers();
        // Add a peer to the list
        void add_peer(struct message *);
        // Remove a peer from the list
        void remove_peer(struct client_info *);

        // Put together a message containing the given data
        void construct_message(msg_t type, struct message *dest, const void *src, size_t n);
        // Create a tcp socket
        int make_socket();

        // Read a message from a tcp socket into 'msg'
        ssize_t read_message(int sock, struct message *msg);
        // Send a message over a tcp socket
        ssize_t send_message(int sock, struct message *msg);

        /*
         * The server, and associated helper functions
         */
        // Threading wrappers
        static void *start_server(void *);
        static void *do_forward(void *);

        // Wait for and handle network requests from other peers
        void accept_connections();

        // Send a list of all peers to a new member
        void handle_join(int client_sock, struct message *in_msg);
        // Respond to a ping request
        void handle_ping(int client_sock, struct message *in_msg);
        // Deal with a request to add a peer to the network
        void handle_peer(int client_sock, struct message *in_msg);
        // Deal with a quit request
        void handle_quit(struct message *in_msg);
        // Deal with a chunk of a message
        void handle_chunk(int client_sock, struct message *in_msg);

        // Confirm receipt of a message
        void confirm_message(struct message *in_msg);
        // Send a list of chunks to every peer in the group
        void forward(std::list< std::shared_ptr< struct message > > msg_list);

};

struct forward_event {
    int sock;
    std::list< std::shared_ptr< struct message > > msg_list;
    unsigned int peer_id;
    Broadcast_Channel *this_ptr;
};

#endif /* __BROADCAST_CHANNEL_H */
