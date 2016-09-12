CC = gcc
STRIP = strip

INCLUDE := -I.
CFLAGS  := -O0 -g $(INCLUDE) -Wall -Werror
LDFLAGS := -lpthread 

COMMON := common/*.c
CLIENT := client.c common.c  $(COMMON)
SERVER := server.c $(COMMON)

final: all
	@$(STRIP) client
	@$(STRIP) server

all: $(CLIENT) $(SERVER)
	$(CC) $(CFLAGS) $(CLIENT) $(LDFLAGS) -o client
	$(CC) $(CFLAGS) $(SERVER) $(LDFLAGS) -o server
	
%.o : %.c
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf client server *.o
