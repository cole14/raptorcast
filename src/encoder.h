#ifndef __ENCODER_H
#define __ENCODER_H

class Encoder {
    public:
        virtual ~Encoder() { }
        virtual unsigned char * generate_chunk () = 0;
};

#endif /* __ENCODER_H */
