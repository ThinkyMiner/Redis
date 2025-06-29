#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <vector>

using namespace std;

static void msg(const char* msg){
    cerr << msg << endl;
}

static void die(const char* msg){
    int err = errno;
    cerr << "[" << err << "] " << msg << endl;
    abort();
}

static int32_t read_full(int fd, const char* buff, size_t n){
    while(n > 0){
        ssize_t rv = read(fd, (void *)buff, n);
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

static void append_buff(vector<uint8_t> &buf, const uint8_t *data, size_t len){
    buf.insert(buf.end(), data, data + len);
}

const size_t max_msg = 4096;

static int32_t send_req(int fd, const uint8_t *text, size_t len){

    if(len > max_msg){
        cout << "Message too long to send";
        return -1;
    }

    vector<uint8_t> buf;
    uint32_t len32 = (uint32_t)len;
    append_buff(buf, (const uint8_t*)&len32, 4);
    append_buff(buf, text, len);

    return write_full(fd, reinterpret_cast<const char*>(buf.data()), len + 4); // read about this typecasting

}

static int32_t recieve_res(int fd){
    vector<uint8_t>buf;
    buf.resize(4);
    errno = 0;

    int32_t err = read_full(fd, (char *)buf.data(), 4); // get the size of the data recieved

    if(err){
        if(errno == 0){
            msg("EOF");
        }else{
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, buf.data(), 4); // copy that to len

    if(len > max_msg){
        msg("Message too long");
        return -1;  // FIX: Added missing return statement
    }

    buf.resize(len + 4);
    // FIX: Read only 'len' bytes, not 'len + 4'
    err = read_full(fd, (char*)&buf[4], len);

    if(err){
        if(errno == 0){
            msg("EOF");
        }else{
            msg("read() error");
        }
        return err;
    }

    cout << "len : " << len << endl;  // FIX: Added endl

    // FIX: Print only the message part (skip the 4-byte length header)
    for (size_t i = 4; i < buf.size(); ++i) {
        cout << static_cast<char>(buf[i]);
    }
    cout << endl;  // FIX: Added newline after message
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    // FIX: Use htons() instead of ntohs()
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect");
    }

    // FIX: Use smaller test messages first
    vector<string> query_list = {
        "hello1", "hello2", "hello3",
        // a large message requires multiple event loop iterations
        string(max_msg, 'z'),
        "hello5",
    };
    for (const string &s : query_list) {
        int32_t err = send_req(fd, (uint8_t *)s.data(), s.size());
        if (err) {
            goto L_DONE;
        }
    }
    for (size_t i = 0; i < query_list.size(); ++i) {
        int32_t err = recieve_res(fd);
        if (err) {
            goto L_DONE;
        }
    }

L_DONE:
    close(fd);
    return 0;
}