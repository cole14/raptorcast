#ifndef __ENCODER_H
#define __ENCODER_H

class Encoder {
    public:
        virtual ~Encoder() { }
        virtual unsigned char * generate_chunk() = 0;
        virtual void init(unsigned char *data, size_t data_len, size_t chunk_len) = 0;
};

#endif /* __ENCODER_H */
