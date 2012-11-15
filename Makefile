.PHONY: all clean

#Important Directories
BINDIR=./bin
SRCDIR=./src

#Compilation Vars
CC=g++
CFLAGS=-g -Wall -Werror
LDFLAGS=-lpthread -pthread

#List of source files 
SRC_FILES=client.cpp broadcast_channel.cpp channel_listener.cpp client_server_encoder.cpp client_server_decoder.cpp
#List of generated object files
OBJS = $(patsubst %.cpp, $(BINDIR)/%.o, $(SRC_FILES))
#Client executable
CLIENT=$(BINDIR)/client

#Default Rule
all: $(CLIENT)

#Rule to make the generated file directory
$(BINDIR):
	mkdir -p $(BINDIR)

#Rule to make object files
$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CC) -c $(CFLAGS) $< -o $@

#Rule to make final executable
$(CLIENT): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

#Rule to clean the build
clean:
	-rm -rf $(BINDIR)
