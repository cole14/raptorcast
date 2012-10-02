#ifndef __ITER_TCP_CHANNEL_H
#define __ITER_TCP_CHANNEL_H

#include "broadcast_channel.h"
#include "channel_listener.h"

class iter_tcp_channel : public broadcast_channel {
    public:
        //default constructor
        iter_tcp_channel(int port, channel_listener *lstnr);

        //default destructor
        virtual ~iter_tcp_channel(void);

        //Connect to an existing broadcast group
        void connect(std::string& ip_port);

        //Broadcast a message over this channel
        //This is performed by sequentially sending the entire contents of the file
        //to each client over a tcp connection.
        void send(unsigned char *buf, size_t buf_len);
};

#endif /* __ITER_TCP_CHANNEL_H */
