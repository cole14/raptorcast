#include "iter_tcp_channel.h"

int main(int argc, char *argv[]){

    //Create and destroy an iterative tcp broadcast channel on port 44123
    iter_tcp_channel *chan = new iter_tcp_channel(44123);
    delete chan;

    return 0;
}

