#include "broadcast_channel.h"

//Default constructor - initialize the port member
broadcast_channel::broadcast_channel(int port, channel_listener *lstnr)
:port(port),
listener(lstnr),
members()
{ }

//Default destructor - nothing to do
broadcast_channel::~broadcast_channel(void)
{ }

