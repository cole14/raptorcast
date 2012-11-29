#ifndef __ENCODER_H
#define __ENCODER_H

#include "message_types.h"

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
