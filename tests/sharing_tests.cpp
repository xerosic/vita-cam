#include "core.hpp"
#include "sharing.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sys/socket.h>
#include <unistd.h>

namespace {
struct Endpoint {
    int port;
    std::string prefix;
    explicit Endpoint(const std::string& url) {
        const size_t colon = url.find(':',7), slash = url.find('/',colon);
        port = std::stoi(url.substr(colon+1,slash-colon-1)); prefix = url.substr(slash);
    }
    int connect() const {
        const int client = socket(AF_INET,SOCK_STREAM,0); assert(client >= 0);
        sockaddr_in address{}; address.sin_family = AF_INET; address.sin_port = htons(port);
        inet_pton(AF_INET,"127.0.0.1",&address.sin_addr);
        assert(::connect(client,reinterpret_cast<sockaddr*>(&address),sizeof(address)) == 0);
        timeval timeout{5,0}; setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        return client;
    }
    std::string request(const std::string& target,const std::string& method = "GET") const {
        const int client = connect();
        const std::string wire = method+" "+target+" HTTP/1.1\r\nHost: localhost\r\n\r\n";
        assert(send(client,wire.data(),wire.size(),0) == long(wire.size()));
        std::string response; char data[32768];
        for (;;) { const auto got = recv(client,data,sizeof(data),0); assert(got >= 0); if (!got) break; response.append(data,size_t(got)); }
        close(client); return response;
    }
};
}
int main() {
    char temporary[] = "/tmp/vitacam-share-XXXXXX";
    const std::string root = mkdtemp(temporary);
    std::vector<cam::Pixel> pixels(cam::Width*cam::Height,{65,120,180,255});
    std::string error;
    assert(cam::writePng(root+"/test.png",pixels,error));
    assert(cam::writePng(root+"/test-Warm.png",pixels,error));
    std::ofstream(root+"/unrelated.txt") << "not a photo";
    cam::PhotoShare share;
    assert(share.start(root,error)); assert(error.empty() && share.active() && share.connected());
    const Endpoint endpoint(share.url());
    const auto qr = cam::makeQr(share.url());
    assert(qr.size >= 21 && qr.modules.size() == size_t(qr.size*qr.size));
    const auto page = endpoint.request(endpoint.prefix);
    assert(page.find("HTTP/1.1 200 OK") == 0 && page.find("test-Warm.png") != std::string::npos);
    assert(page.find("unrelated.txt") == std::string::npos);
    const auto photo = endpoint.request(endpoint.prefix+"photo/test-Warm.png?download=1");
    assert(photo.find("Content-Disposition: attachment; filename=\"test-Warm.png\"") != std::string::npos);
    std::ifstream file(root+"/test-Warm.png",std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    assert(photo.substr(photo.find("\r\n\r\n")+4) == bytes);
    const auto head = endpoint.request(endpoint.prefix+"photo/test.png","HEAD");
    assert(head.find("200 OK") != std::string::npos && head.size() == head.find("\r\n\r\n")+4);
    for (const auto& path : {std::string("/"), endpoint.prefix+"photo/../unrelated.txt",endpoint.prefix+"photo/%2e%2e/test.png",
            endpoint.prefix+"photo/missing.png",endpoint.prefix+"photo/test.png?download=2"})
        assert(endpoint.request(path).find("404 Not Found") != std::string::npos);
    assert(endpoint.request(endpoint.prefix,"POST").find("405 Method Not Allowed") != std::string::npos);
    assert(endpoint.request(endpoint.prefix+"?page=-1").find("400 Bad Request") != std::string::npos);
    const int stalled = endpoint.connect();
    assert(send(stalled,"GET ",4,0) == 4);
    const auto start = std::chrono::steady_clock::now();
    share.stop();
    assert(std::chrono::steady_clock::now()-start < std::chrono::seconds(1));
    close(stalled);
    assert(!share.active() && share.url().empty());
    assert(share.start(root,error));
    const Endpoint restarted(share.url());
    assert(restarted.prefix != endpoint.prefix);
    assert(restarted.request(endpoint.prefix).find("404 Not Found") != std::string::npos);
    share.stop();
    std::filesystem::remove_all(root);
}
