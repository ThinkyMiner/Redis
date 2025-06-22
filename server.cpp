#include <iostream>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

using namespace std;


static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

const size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    fprintf(stderr, "client says: %.*s\n", len, &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = ::bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue;   // error
        }

        while (true) {
            // here the server only serves one client connection at once
            int32_t err = one_request(connfd);
            if (err) {
                break;
            }
        }
        close(connfd);
    }

    return 0;
}

int mainold() {
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