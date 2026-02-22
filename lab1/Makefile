.PHONY: all clean


all: client server

client: client1.cpp
	g++ client1.cpp -Wall -Wextra -o client

server: server1.cpp
	g++ server1.cpp -Wall -Wextra -o server

clean:
	rm -f client server