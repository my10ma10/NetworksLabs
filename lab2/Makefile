.PHONY: all clean

all: server/server client/client

server/server: messaging.o server/server.cpp server/server.cpp server/main.cpp
	g++ server/main.cpp server/server.cpp messaging.o -Wall -Wextra -o server/server

client/client: messaging.o client/client.cpp client/client.cpp client/main.cpp
	g++ client/main.cpp client/client.cpp messaging.o -Wall -Wextra -o client/client

messaging.o: messaging.hpp messaging.cpp
	g++ -c messaging.cpp -o messaging.o

clean:
	rm -f client/client server/server messaging.o