#ifndef __DECODER_H
#define __DECODER_H

#include <vector>
#include <map>

#include "enc/encoder.h"
#include "message_types.h"
#include "logger.h"


class Decoder;

class Decoder_Context {
    public :
        virtual void fill_block_list(std::vector<unsigned> *dest) = 0;
        virtual void fill_chunk_list(std::vector<unsigned> *dest) = 0;
        virtual unsigned char *get_block(unsigned index) = 0;
        virtual unsigned char *get_chunk(unsigned index) = 0;

        virtual void set_block(unsigned char *data, unsigned index) = 0;

        virtual Message_Descriptor *get_descriptor() = 0;
};

class Incoming_Message : public Decoder_Context {
    public :
        // Destructor
        virtual ~Incoming_Message();

        // B_Chan interface
        Incoming_Message(msg_t algo);
        void add_chunk(unsigned char *data, size_t len, int chunk_id);
        bool is_ready();
        bool should_forward();
        size_t get_len();
        unsigned char *get_message();

        // Decoder interface
        void fill_block_list(std::vector<unsigned> *dest);
        void fill_chunk_list(std::vector<unsigned> *dest);
        unsigned char *get_block(unsigned index);
        unsigned char *get_chunk(unsigned index);

        void set_block(unsigned char *data, unsigned index);

        Message_Descriptor *get_descriptor();

    private :
        // Before the message is decoded, working data is stored here
        std::map<unsigned, unsigned char *> blocks;
        std::map<unsigned, unsigned char *> chunks;

        Message_Descriptor *descriptor;

        Decoder *get_decoder(msg_t algo);
        Decoder *decoder;

        void print_lists();

};

class Decoder {
    public:
        Decoder(Decoder_Context *ctx) : context(ctx) { }
        virtual ~Decoder() { }
        virtual void notify (unsigned chunk_id) = 0;
        virtual bool is_ready () = 0;
        virtual bool should_forward() = 0;

    protected:
        Decoder_Context *context;
};

#endif /* __DECODER_H */
