#ifndef __TEST_CLIENT_H
#define __TEST_CLIENT_H

#include "channel_listener.h"
#include "broadcast_channel.h"

class TestClient : public Channel_Listener {
    public:
        TestClient(const char *config_filename);
        ~TestClient(void);

        void receive(unsigned char *buf, size_t buf_len);
        void test(void);
    private:
        Broadcast_Channel *chan;
        std::string name;
};

#endif /* __TEST_CLIENT_H */
