#ifndef __CLIENT_H
#define __CLIENT_H

#include "channel_listener.h"
#include "broadcast_channel.h"

class Client : public Channel_Listener {
    public:
        Client(std::string name, std::string port);
        ~Client();

        void receive(unsigned char *buf, size_t buf_len);
        void connect();
        void run_cli();

    private:
        //helper stuffs
        void print_prompt();
        std::string read_hostname(void);
        char *read_stripped_line(void);
        int read_port(void);
        char *line_buf;
        size_t line_buf_len;
        ssize_t line_buf_filled_len;

        Broadcast_Channel *chan;
        std::string name;

        msg_t get_alg();
};

#endif /* __CLIENT_H */
