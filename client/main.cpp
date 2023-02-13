#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

void receiveMessages(int socket)
{
    while (true) {
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));

        int n = recv(socket, buffer, sizeof(buffer), 0);
        if (n == 0) {
            // Connection closed by the server
            break;
        }
        else if (n < 0) {
            // Error occurred during receive
            std::cerr << "Error occurred during receive: " << strerror(errno) << std::endl;
            break;
        }

        std::cout << "Received message: " << buffer << std::endl;
    }
}

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(1100);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    int result = connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (result < 0) {
        std::cerr << "Error connecting to server: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Connected to server" << std::endl;

    // Create a new thread to receive messages from the server
    std::thread receiveThread(receiveMessages, clientSocket);

    while (true) {
        std::string message;
        std::getline(std::cin, message);

        int n = send(clientSocket, message.c_str(), message.length(), 0);
        if (n < 0) {
            std::cerr << "Error sending message: " << strerror(errno) << std::endl;
            break;
        }
    }

    receiveThread.join();

    close(clientSocket);

    return 0;
}
