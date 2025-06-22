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
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); // AF_INET -> IPv4, AF_INET6 -> IPv6, SOCK_STREAM -> TCP or ByteStream, SOCK_DGRAM -> UDP
    cout << serverSocket;
    if (serverSocket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { // SOL_SOCKET -> means setting options at socket API Level, SO_REUSEADDR -> Avoid Address already in use errors when server restarted like gives back the resources to the system
        cerr << "Error setting socket options" << endl;
        return 1;
    }

    // struct sockaddr_in {
    //     uint16_t       sin_family; // AF_INET
    //     uint16_t       sin_port;   // port in big-endian
    //     struct in_addr sin_addr;   // IPv4
    // };

    // struct in_addr {
    //     uint32_t       s_addr;     // IPv4 in big-endian
    // };
    
    // Configure server address
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    // Bind socket - Use global namespace resolution operator to avoid name collision
    int rv = ::bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)); // size of serverAddr is required as the ip6 struct has a different size 
    if (rv) {
        cerr << "Error binding socket" << endl;
        return 1;
    }
    
    // Listen for connections
    if (listen(serverSocket, 5) < 0) {
        cerr << "Error listening" << endl;
        return 1;
    }
    
    cout << "Server started. Listening on port 8080..." << endl;
    
    // Accept connections and handle them
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize); // this is where it becomes a connection socket
        
        if (clientSocket < 0) {
            cerr << "Error accepting connection" << endl;
            continue;
        }
        
        // Get client info
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        cout << "Client connected: " << clientIP << endl;
        
        // Receive and send messages
        char buffer[1024] = {0};
        int bytesReceived = recv(clientSocket, buffer, 1024, 0);
        if (bytesReceived > 0) {
            cout << "Received: " << buffer << endl;
            
            // Echo back the message
            string response = "Server received: ";
            response += buffer;
            send(clientSocket, response.c_str(), response.length(), 0);
        }
        
        close(clientSocket);
    }
    
    // Close server socket
    close(serverSocket);
    return 0;
}