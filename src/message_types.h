#ifndef __MESSAGE_TYPES_H
#define __MESSAGE_TYPES_H

#include <stddef.h>

#define PACKET_LEN 256
#define MAX_NAME_LEN 128  // Has to fit w/in PACKET_LEN

enum msg_t {
    // Functional messages
    JOIN,   // Initial message sent to known host
    PEER,   // Tell about a peer (this or another)
    READY,
    QUIT,
    CONFIRM,
    // Broadcast algorithms
    CLIENT_SERVER,
    TRAD,
    COOP,
    RAPTOR
};

struct message {
    msg_t type;
    unsigned int cli_id;
    unsigned int msg_id;
    unsigned int chunk_id;
    unsigned int ttl;
    size_t data_len;
    unsigned char data[PACKET_LEN];
};

#endif /* __MESSAGE_TYPES_H */
