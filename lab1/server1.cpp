#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <unistd.h>
#include <cstring>

#include <iostream>

#define BUF_SIZE 1024
#define PORT 8080

constexpr bool is_active = true;

int main() {
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in serverAddr, clientAddr;

    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(sock_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    char buffer[BUF_SIZE];
    std::memset(buffer, 0, BUF_SIZE);

    while (is_active) {
        socklen_t addrLen = sizeof(clientAddr);

        int n = recvfrom(
            sock_fd, buffer, BUF_SIZE, 0, 
            (struct sockaddr*)&clientAddr, &addrLen
        );
        if (n < 0) {
            close(sock_fd);
            std::exit(1);
        }
        buffer[n] = '\0';
        
        std::cout << "SERVER: client msg: " << buffer << std::endl;

        sendto(sock_fd, buffer, n, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    }
    close(sock_fd);
    return 0;
}