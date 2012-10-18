raptorcast
==========

This is the repo for the O(N) reliable broadcast algorithm for the google coding competition.

The 'client' program will be a sort of file chatroom, where users will run the client, bootstrap their way into the broadcast group, then
broadcast files, or store files broadcasted by the group into files into the current working directory.

bootstrap method
==========

The way a user will join the broadcast group is that they will first create their broadcast channel in order to initialize their
client_info struct. This will populate the struct with the user's name, the externally visible ip at which other users in the group will
contact the new user, the port on which the user will be listening, and client id '0' to represent a not-yet joined user. The client will
then request the hostname/port of an existing member of the broadcast group to connect to in order to request membership with the group. If
the input hostname is the empty string, then the client will start the broadcast group itself, and initialize the groupset with itself.
Otherwise, the client will then connect to that existing member and initiate the bootstrap protocol.  The client will first send a 'request to join'
message which will include its client_info struct.  The existing member will then assign it a client id and proceed to broadcast its
membership to the rest of the group.  Once the rest of the group has acknowledged the request to join, the existing member will respond to
the new user with its assigned client id and existing group membership.  The client will then establish a connection with each member in
the group, upon which the member will finally include them in their group membership.

It is important to note that this bootstrap method alone will not ensure that every node in the group knows about every other node -- there
exist pathological cases where another node tries to join when a node is already trying to join, and the ordering of messages can mean that
one of the new nodes may not know about the other.  We could spend time getting the bootstrap method to work, but instead we'll just
manually join members to the group in a non-disruptive way.


