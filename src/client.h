
#include "channel_listener.h"
#include "broadcast_channel.h"

class client : public channel_listener {
    public:
        client(std::string name, int port);
        ~client();

        void receive(unsigned char *buf, size_t buf_len);

    private:
        broadcast_channel *chan;
        std::string name;
};

