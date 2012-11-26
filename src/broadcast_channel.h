#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <netinet/ip.h>

#include <stddef.h>
#include <string>
#include <vector>
#include <map>

#include "channel_listener.h"
#include "decoder.h"
#include "encoder.h"

#define PACKET_LEN 256
#define MAX_NAME_LEN 128  // Has to fit w/in PACKET_LEN

struct client_info {
    struct sockaddr_in ip;
    unsigned int id;
    char name[MAX_NAME_LEN];
};

enum msg_t {
    // Functional messages
    JOIN,   // Initial message sent to known host
    PEER,   // Tell about a peer (this or another)
    READY,
    QUIT,
    // Broadcast algorithms
    CLIENT_SERVER,
    TRAD,
    COOP,
    RAPTOR
};

struct message {
    msg_t type;
    unsigned int cli_id;
    unsigned int msg_id;
    unsigned int chunk_id;
    size_t data_len;
    unsigned char data[PACKET_LEN];
};

class broadcast_channel {
    public:
        // Constructor
        broadcast_channel(std::string name, std::string port, channel_listener *lstnr);

        // Default destructor
        virtual ~broadcast_channel(void);

        // Connect to an existing broadcast group
        bool join(std::string hostname, int port);

        // Broadcast a message over this channel
        void broadcast(msg_t algo, unsigned char *buf, size_t buf_len);

        // Print the list of known peers at indentation level 'indent' to stdout
        void print_peers(int indent = 0);

        // Notify peers that we're quitting, clean up connections, etc.
        void quit();

    private:
        // The client info for this broadcast_channel
        struct client_info *my_info;
        // The client info for everyone in the broadcast group
        std::vector< struct client_info * > group_set;
        // Association between message_ids and their active decoders
        std::map< unsigned long, decoder * > decoders;
        // The chunk receiver thread
        pthread_t receiver_thread;
        // The application listening on this channel
        channel_listener *listener;
        // The monotonically increasing unique message id counter for this sender
        unsigned long msg_counter;

        // Contact a known host and get a list of all peers
        bool get_peer_list(std::string hostname, int port);
        // Send a list of all peers to a new member
        bool send_peer_list(int client_sock, struct client_info *target);
        // Tell the list of peers that we exist
        bool notify_peers();
        // Add a peer to the list
        void add_peer(struct message *);
        // Wait for and handle network requests from other peers
        void accept_connections();
        // Wrapper func for start_server for the purpose of threadding
        static void *start_server(void *);

        // Get an encoder object for message type 'algo'
        encoder *get_encoder(msg_t algo);
        // Construct a decoder of the appropriate type
        decoder *get_decoder(msg_t);

        // Get an id that is not currently in use within the peer set
        // Note that this is terrible and presents all sorts of race conditions
        unsigned int get_unused_id();
        // Put together a message containing the given data
        void construct_message(msg_t type, struct message *dest, const void *src, size_t n);
};

#endif /* __BROADCAST_CHANNEL_H */
