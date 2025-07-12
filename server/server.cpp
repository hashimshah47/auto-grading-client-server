#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <vector>

#define PORT 8080

void sendFile(int client_sock, const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Cannot open " << filename << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    int size = file.tellg();
    file.seekg(0);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);

    send(client_sock, &size, sizeof(int), 0);
    send(client_sock, buffer.data(), size, 0);
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 1);

    std::cout << "Waiting for client..." << std::endl;
    client_sock = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    std::cout << "Client connected!" << std::endl;

    sendFile(client_sock, "logo.png");
    sendFile(client_sock, "Page");  // Must be built beforehand

    close(client_sock);
    close(server_fd);
}
// g++ server.cpp -o server
// ./server