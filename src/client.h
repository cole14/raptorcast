#ifndef __CLIENT_H
#define __CLIENT_H

#include "channel_listener.h"
#include "broadcast_channel.h"

class client : public channel_listener {
    public:
        client(std::string name, std::string port);
        ~client();

        void receive(unsigned char *buf, size_t buf_len);
        void connect();
        void run_cli();

    private:
        broadcast_channel *chan;
        std::string name;

        msg_t get_alg();
};

#endif /* __CLIENT_H */
