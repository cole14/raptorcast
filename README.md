raptorcast
==========

This is the repo for the O(N) reliable broadcast algorithm for the google coding competition.

The 'client' program will be a sort of file chatroom, where users will run the client, bootstrap their way into the broadcast group, then
broadcast files, or store files broadcasted by the group into files into the current working directory.

bootstrap method
==========

The way a user will join the broadcast group is that they will first connect to a node which currently exists in the group. That node will
then notify the existing members of the group that 'new_user' is trying to join.  After notifying the members of the group, the bootstrap
node will respond to 'new_user's request with the list of all the members currently in the group.  'new_user' will then establish a
connection with each of the users in the group, upon which each of the existing users will officially include them in the broadcast group.

It is important to note that this bootstrap method alone will not ensure that every node in the group knows about every other node -- there
exist pathological cases where another node tries to join when a node is already trying to join, and the ordering of messages can mean that
one of the new nodes may not know about the other.  We could spend time getting the bootstrap method to work, but instead we'll just
manually join members to the group in a non-disruptive way.


