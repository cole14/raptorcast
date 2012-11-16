#ifndef __ENCODER_H
#define __ENCODER_H

class encoder {
    public:
        virtual ~encoder() { }
        virtual unsigned char * generate_chunk() = 0;
        virtual void init(unsigned char *data, size_t data_len, size_t chunk_len, size_t num_peers) = 0;
        virtual void next_stream() = 0;
};

#endif /* __ENCODER_H */
