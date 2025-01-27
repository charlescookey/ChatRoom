#pragma once


#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>


#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_BUFFER_SIZE 1024

class Network {
    const char* host = "127.0.0.1"; // Server IP address
    unsigned int port = 65432;
    std::string sentence = "Hello, server!";
    SOCKET client_socket = socket(0,0,0);
public:
    Network() {
    }

    ~Network() {
        cleanup();
    }

    int initialize(){
        // Initialize WinSock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << std::endl;
            return -1;
        }

        // Create a socket
        client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return -1;
        }

        // Resolve the server address and port
        sockaddr_in server_address = {};
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        if (inet_pton(AF_INET, host, &server_address.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            closesocket(client_socket);
            WSACleanup();
            return -1;
        }

        // Connect to the server
        if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == SOCKET_ERROR) {
            std::cerr << "Connection failed with error: " << WSAGetLastError() << std::endl;
            closesocket(client_socket);
            WSACleanup();
            return -1;
        }
        return 0;
    }

    int sendMessage(std::string message) {
        if (send(client_socket, message.c_str(), static_cast<int>(sentence.size()), 0) == SOCKET_ERROR) {
            std::cerr << "Send failed with error: " << WSAGetLastError() << std::endl;
            closesocket(client_socket);
            WSACleanup();
            return -1;
        }
        return 0;
    }

    int receiveMessage(std::string &message) {
        char buffer[DEFAULT_BUFFER_SIZE] = { 0 };
        int bytes_received = recv(client_socket, buffer, DEFAULT_BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0'; // Null-terminate the received data
            //std::cout << "Received from server: " << buffer << std::endl;
            message = std::string(buffer);
        }
        else if (bytes_received == 0) {
            std::cout << "Connection closed by server." << std::endl;
            return -1;
        }
        else {
            std::cerr << "Receive failed with error: " << WSAGetLastError() << std::endl;
            return -1;
        }
        return 0;
    }

    void cleanup() {
        closesocket(client_socket);
        WSACleanup();
    }
};