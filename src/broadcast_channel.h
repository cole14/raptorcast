#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <stddef.h>
#include <string>
#include <vector>

#include "channel_listener.h"

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
