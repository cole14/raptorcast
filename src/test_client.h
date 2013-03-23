#ifndef __TEST_CLIENT_H
#define __TEST_CLIENT_H

#include "channel_listener.h"
#include "broadcast_channel.h"

class TestClient : public Channel_Listener {
    public:
        TestClient(std::string myport, int groupport, bool master);
        ~TestClient(void);

        void receive(unsigned char *buf, size_t buf_len);
        void test(void);
    private:
        Broadcast_Channel *chan;
        std::string name;
        int groupport;
        bool master;
};

#endif /* __TEST_CLIENT_H */
