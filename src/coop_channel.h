#ifndef __COOP_CHANNEL_H
#define __COOP_CHANNEL_H

#include "broadcast_channel.h"

class coop_channel : public broadcast_channel {
    public:
        //default constructor
        coop_channel(int port);

        //default destructor
        virtual ~coop_channel(void);

        //Connect to an existing broadcast group
        void connect(std::string& ip_port);

        //Broadcast a message over this channel
        //This is performed by sequentially sending F/N of the file
        //to each client, who in turn rebroadcast it over a tcp connection.
        void send(unsigned char *buf, size_t buf_len);

        //Receive a message which was broadcasted
        //Returns the number of bytes left unread
        size_t recv(unsigned char *buf, size_t buf_len);
};

#endif /* __COOP_CHANNEL_H */
