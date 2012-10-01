#ifndef __RAPTOR_CHANNEL_H
#define __RAPTOR_CHANNEL_H

#include "broadcast_channel.h"

class raptor_channel : public broadcast_channel {
    public:
        //default constructor
        raptor_channel(int port);

        //default destructor
        virtual ~raptor_channel(void);

        //Broadcast a message over this channel
        //This is performed by sequentially sending 2F/N of the file size as raptor code chunks
        //to each client, who in turn rebroadcast them over a tcp connection.
        void send(unsigned char *buf, size_t buf_len);

        //Receive a message which was broadcasted
        //Returns the number of bytes left unread
        size_t recv(unsigned char *buf, size_t buf_len);
};

#endif /* __RAPTOR_CHANNEL_H */

