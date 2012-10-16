#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <stddef.h>
#include <string>
#include <vector>

#include "channel_listener.h"

#define PACKET_LEN 256;

struct client_info {
    Std::String name;
    Struct sockaddr_in *ip;
    unsigned in id;
};

enum algo_t {
    CLIENT_SERVER,
    TRAD,
    COOP,
    RAPTOR
};

struct message {
    algo_t type;
    unsigned int cli_id;
    unsigned long msg_id;
    int data_len;
    unsigned char [PACKET_LEN] data;
};

class broadcast_channel {
    public:
        broadcast_channel(int port, channel_listener *lstnr);

        //Default destructor
        virtual ~broadcast_channel(void);

        //Connect to an existing broadcast group
        virtual void connect(std::string& ip_port) = 0;

        //Broadcast a message over this channel
        virtual void send(unsigned char *buf, size_t buf_len) = 0;

    private:
        int port;
        channel_listener *listener;
        std::vector< std::string > members;
};

#endif /* __BROADCAST_CHANNEL_H */
