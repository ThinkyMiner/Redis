#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

static void msg(const char* msg){
    cerr << msg << endl;
}

static void die(const char* msg){
    int err = errno;
    cerr << "[" << err << "] " << msg << endl;
    abort();
}

static int32_t read_full(int fd, char* buff, size_t n){
    while(n > 0){
        ssize_t rv = read(fd, buff, n);
        if(rv <= 0){
            return -1;
        }
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

static int32_t write_full(int fd, const char* buff, size_t n){
    while(n > 0){
        ssize_t rv = write(fd, buff, n);
        if(rv <= 0){
            return -1;
        }
        n -= (size_t)rv;
        buff += rv;
    }
    return 0;
}

const size_t max_msg = 4096;

static int32_t query(int fd, const char* text){

    uint32_t len = (uint32_t)strlen(text);

    if(len > max_msg){
        cerr << "Message too long";
        return -1;
    }

    char wbuff[4 + max_msg];
    memcpy(wbuff, &len, 4); // size of the message
    memcpy(&wbuff[4], text, len); // The Message

    int32_t err = write_full(fd, wbuff, len + 4);
    if(err){ // length till the message is present not the max length
        cerr << "Error writing" << endl;
        return err;
    }

    char rbuf[4 + max_msg + 1];
    errno = 0;
    err = read_full(fd, rbuf, 4);

    if(err){
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, rbuf, 4);
    if(len > max_msg){ // Not required but good practice
        msg("Message too long");
        return -1;
    }

    err = read_full(fd, &rbuf[4], len);
    if(err){
        msg("read() error");
        return err;
    }

    printf("server says: %.*s\n", len, &rbuf[4]);

    return 0;

}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    // multiple requests
    int32_t err = query(fd, "hello1");
    if (err) {
        goto L_DONE;
    }
    err = query(fd, "hello2");
    if (err) {
        goto L_DONE;
    }
    err = query(fd, "hello3");
    if (err) {
        goto L_DONE;
    }

L_DONE:
    close(fd);
    return 0;
}