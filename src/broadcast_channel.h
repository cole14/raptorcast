#ifndef __BROADCAST_CHANNEL_H
#define __BROADCAST_CHANNEL_H

#include <stddef.h>

class broadcast_channel {
    public:
        broadcast_channel();

        //Default destructor
        virtual ~broadcast_channel(void);

        //Broadcast a message over this channel
        virtual void send(unsigned char *buf, size_t buf_len) = 0;

        //Receive a message which was broadcasted
        //Returns the number of bytes left unread
        virtual size_t recv(unsigned char *buf, size_t buf_len) = 0;
};

#endif /* __BROADCAST_CHANNEL_H */
