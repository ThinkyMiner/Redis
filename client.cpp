#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

int main() {
    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }
    
    // Configure server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    
    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid address" << endl;
        return 1;
    }
    
    // Connect to the server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Connection failed" << endl;
        return 1;
    }
    
    cout << "Connected to server at 127.0.0.1:8080" << endl;
    
    // Get message from user
    string message;
    cout << "Enter message to send: ";
    getline(cin, message);
    
    // Send message
    send(clientSocket, message.c_str(), message.length(), 0);
    cout << "Message sent" << endl;
    
    // Receive response
    char buffer[1024] = {0};
    int bytesReceived = recv(clientSocket, buffer, 1024, 0);
    if (bytesReceived > 0) {
        cout << "Server response: " << buffer << endl;
    }
    
    // Close socket
    close(clientSocket);
    return 0;
}