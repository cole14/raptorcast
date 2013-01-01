#ifndef __ENCODER_H
#define __ENCODER_H

#include <vector>

#include "message_types.h"

// Interface between the outgoing message and the encoder
class Encoder_Context {
    public:
        virtual char *get_block(unsigned index) = 0;
        virtual size_t get_num_blocks() = 0;
        virtual size_t get_block_len() = 0;
        virtual size_t get_num_peers() = 0;
        // Additional accessors as needed
};

// Interface between the outgoing message and the broadcast channel
// XXX better (more illustrative) name
class Encoder_Interface {
    public :
        virtual std::vector<char *> *get_chunks(unsigned peer) = 0;
};

class Outgoing_Message : public Encoder_Context, public Encoder_Interface {
    public :
        // B_Chan interface
        Outgoing_Message(char *data, size_t d_len, size_t n_peers, size_t c_len);
        std::vector<char *> *get_chunks(unsigned peer);

        // Encoder iface
        char *get_block(unsigned index);
        size_t get_num_blocks();
        size_t get_block_len();
        size_t get_num_peers();

    private :
        char *data;
        size_t data_len;
        size_t num_peers;
        size_t chunk_len;
};


class encoder {
    public:
        virtual ~encoder() { }
        virtual int generate_chunk(unsigned char **dest, unsigned int *chunk_id) = 0;
        virtual void init(unsigned char *data, size_t data_len, size_t chunk_len, size_t num_peers) = 0;
        virtual void next_stream() = 0;
};

// Get an encoder object for message type 'algo'
encoder *get_encoder(msg_t algo);

#endif /* __ENCODER_H */
