#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <chrono>
#include <unistd.h>

void receiveMessages(int sock) {
    char buffer[4096];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Disconnected from server or error receiving data\n";
            break;
        }
        std::cout << "Server: " << std::string(buffer, bytes_received) << std::endl;
    }

    close(sock);
    exit(0);
}

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == -1) {
        std::cerr << "Could not create socket\n";
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(54000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int connection_result = connect(sock, (sockaddr*)&server_addr, sizeof(server_addr));
    if (connection_result == -1) {
        std::cerr << "Connection failed\n";
        return -1;
    }

    std::cout << "Connected to server\n";

    // Start receiving messages in a separate thread
    std::thread(receiveMessages, sock).detach();

    // KEEP THE MAIN THREAD ALIVE HERE
    // For example, simple infinite loop to keep the client running:
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}


