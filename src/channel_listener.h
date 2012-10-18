#ifndef __CHANNEL_LISTENER_H
#define __CHANNEL_LISTENER_H

#include <stddef.h>

class channel_listener {
    public:
        virtual ~channel_listener();

        //This method is invoked by the broadcast channel upon final reception of a message.
        virtual void receive(unsigned char *buf, size_t buf_len) = 0;
};

#endif /* __CHANNEL_LISTENER_H */
