#ifndef __CHANNEL_LISTENER_H
#define __CHANNEL_LISTENER_H

#include <stddef.h>

class channel_listener {
    public:
        virtual void receive(unsigned char *buf, size_t buf_len) = 0;
        virtual ~channel_listener();
};

#endif /* __CHANNEL_LISTENER_H */
