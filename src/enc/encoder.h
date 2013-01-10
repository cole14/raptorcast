#ifndef __ENCODER_H
#define __ENCODER_H

#include <vector>

#include "../message_types.h"

// Forward declaration for use in Outgoing_Message
class Encoder;

// Interface between the outgoing message and the encoder
class Encoder_Context {
    public:
        virtual unsigned char *get_block(unsigned index) = 0;
        virtual size_t get_num_blocks() = 0;
        virtual size_t get_block_len() = 0;
        virtual size_t get_num_peers() = 0;
        // Additional accessors as needed
};

class Outgoing_Message : public Encoder_Context {
    public :
        // Destructor
        ~Outgoing_Message();

        // B_Chan interface
        Outgoing_Message(msg_t algo, unsigned char *data, size_t d_len, size_t n_peers, size_t c_len);
        std::vector< std::pair<unsigned, unsigned char *> > *get_chunks(unsigned peer);

        // Encoder iface
        unsigned char *get_block(unsigned index);
        size_t get_num_blocks();
        size_t get_block_len();
        size_t get_num_peers();

    private :
        unsigned char *padded_data;
        std::vector<unsigned char*> blocks;
        size_t data_len;
        size_t num_peers;
        size_t num_blocks;
        size_t chunk_len;

        Encoder *encoder;

        // Pad the data and split it up into chunks
        void split_blocks(unsigned char *data, size_t data_len);

        // Get an encoder object for message type 'algo'
        Encoder *get_encoder(msg_t algo);
};


class Encoder {
    public:
        Encoder(Encoder_Context *ctx) : context(ctx) { }
        virtual ~Encoder() { }
        virtual std::vector<unsigned> *get_chunk_list(unsigned peer) = 0;
        virtual size_t get_chunk(unsigned char **dest, unsigned chunk_id) = 0;

    protected:
        Encoder_Context *context;
};


#endif /* __ENCODER_H */
