.PHONY: all clean

#Important Directories
BINDIR=./bin
SRCDIR=./src

#Compilation Vars
CC=g++
CFLAGS=-g -Wall -Werror -std=c++0x
LDFLAGS=-lpthread -pthread -lrt

#List of source files 
SRC_FILES=	broadcast_channel.cpp
SRC_FILES+=	channel_listener.cpp
SRC_FILES+=	enc/client_server_encoder.cpp dec/client_server_decoder.cpp 
SRC_FILES+= enc/cooperative_encoder.cpp	
SRC_FILES+= lt_selector.cpp enc/lt_encoder.cpp dec/lt_decoder.cpp 
SRC_FILES+=	dec/decoder.cpp enc/encoder.cpp
SRC_FILES+=	logger.cpp

#List of generated object files
OBJS = $(patsubst %.cpp, $(BINDIR)/%.o, $(SRC_FILES))
#Client executable
CLIENT=$(BINDIR)/client
LT_TEST=$(BINDIR)/lt_test
TEST_CLIENT=$(BINDIR)/test_client

#Default Rule
all: $(CLIENT)

lt_test: $(LT_TEST)

test_client: $(TEST_CLIENT)

#Rule to make the generated file directory
$(BINDIR):
	mkdir -p $(BINDIR)
	mkdir -p $(BINDIR)/enc
	mkdir -p $(BINDIR)/dec

#Rule to make object files
$(BINDIR)/%.o: $(SRCDIR)/%.cpp | $(BINDIR)
	$(CC) -c -I$(SRCDIR) $(CFLAGS) $< -o $@

#Rule to make final executable
$(CLIENT): $(BINDIR)/client.o $(OBJS)
	$(CC) -I$(SRCDIR) $(CFLAGS) $^ $(LDFLAGS) -o $@


# Rules for lt_test
bin/lt_test.o: src/lt_test.cpp | $(BINDIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(LT_TEST): bin/logger.o bin/lt_test.o bin/lt_selector.o bin/lt_encoder.o bin/lt_decoder.o
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# Rules for test_client
bin/test_client.o: src/test_client.cpp | $(BINDIR)
	$(CC) -I$(SRCDIR) -c $(CFLAGS) $< -o $@

$(TEST_CLIENT): bin/test_client.o $(OBJS)
	$(CC) -I$(SRCDIR) $(CFLAGS) $^ $(LDFLAGS) -o $@

#Rule to clean the build
clean:
	-rm -rf $(BINDIR)
