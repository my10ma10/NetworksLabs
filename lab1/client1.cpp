#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <unistd.h>
#include <cstring>

#include <iostream>
#include <string>

#define BUF_SIZE 1024
#define PORT 8080
#define SERVER_IP "127.0.0.1"

constexpr bool is_active = true;

std::string enterMsg();

int main() {
    std::string msg = enterMsg();

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr;

    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    

    char buffer[BUF_SIZE];
    std::memset(buffer, 0, BUF_SIZE);

    while (is_active) {
        sendto(sock_fd, msg.c_str(), msg.size(), 0, 
            (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        socklen_t addrLen = sizeof(serverAddr);

        int n = recvfrom(
            sock_fd, buffer, BUF_SIZE, 0, 
            (struct sockaddr*)&serverAddr, &addrLen
        );
        if (n < 0) {
            close(sock_fd);
            std::exit(1);
        }
        buffer[n] = '\0';

        std::cout << "CLIENT: server msg: " << buffer << std::endl;

        msg = enterMsg();
    }

    close(sock_fd);
    return 0;
}

std::string enterMsg() {
    std::string str;
    std::cout << "Enter message to server: ";
    std::cin >> str;
    std::cout << std::endl;

    return str;
}
