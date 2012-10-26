#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <netinet/ip.h>

#include <stddef.h>
#include <string>
#include <vector>

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
    unsigned long msg_id;
    int data_len;
    unsigned char data[PACKET_LEN];
};

class broadcast_channel {
    public:
        //Constructor
        broadcast_channel(std::string name, int port, channel_listener *lstnr);

        //Default destructor
        virtual ~broadcast_channel(void);

        //Connect to an existing broadcast group
        bool join(std::string hostname, int port);

        //Broadcast a message over this channel
        void broadcast(unsigned char *buf, size_t buf_len);

    private:
        //The client info for this broadcast_channel
        struct client_info my_info;
        //The client info for everyone in the broadcast group
        std::vector< struct client_info * > group_set;
        //The list of currently-active message decoders
        std::vector< Decoder > decoders;
        //The chunk receiver thread
        pthread_t receiver_thread;
        //The application listening on this channel
        channel_listener *listener;
        //The monotonically increasing unique message id counter for this sender
        unsigned long msg_counter;

        bool get_peer_list(std::string hostname, int port);
        bool notify_peers();
};

#endif /* __BROADCAST_CHANNEL_H */
