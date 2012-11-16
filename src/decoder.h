#ifndef __DECODER_H
#define __DECODER_H

class decoder {
    public:
        virtual ~decoder() { }
        virtual void add_chunk (unsigned char * data, size_t len) = 0;
        virtual bool is_done () = 0;
        virtual unsigned char * get_message () = 0;
        virtual size_t get_len () = 0;
};

#endif /* __DECODER_H */
