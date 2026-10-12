#include "sharing.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <set>
#include <sstream>
#include <thread>
#ifdef __vita__
#include "camera.hpp"
#include <psp2/kernel/rng.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/sysmodule.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <random>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace cam {
namespace {
using Clock = std::chrono::steady_clock;
void pauseSocket() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
#ifdef __vita__
using Address = SceNetSockaddrIn;
void closeSocket(int fd) { if (fd >= 0) sceNetSocketClose(fd); }
int acceptSocket(int fd) { return sceNetAccept(fd,nullptr,nullptr); }
int receive(int fd,char* data,size_t size) { return sceNetRecv(fd,data,size,0); }
int transmit(int fd,const char* data,size_t size) { return sceNetSend(fd,data,size,0); }
bool wouldBlock() { const int e = *sceNetErrnoLoc(); return e == SCE_NET_EAGAIN || e == SCE_NET_EINTR; }
bool nonblocking(int fd) { const int on = 1; return sceNetSetsockopt(fd,SCE_NET_SOL_SOCKET,SCE_NET_SO_NBIO,&on,sizeof(on)) >= 0; }
#else
using Address = sockaddr_in;
void closeSocket(int fd) { if (fd >= 0) close(fd); }
int acceptSocket(int fd) { return accept(fd,nullptr,nullptr); }
int receive(int fd,char* data,size_t size) { return int(recv(fd,data,size,0)); }
int transmit(int fd,const char* data,size_t size) {
#ifdef MSG_NOSIGNAL
    return int(send(fd,data,size,MSG_NOSIGNAL));
#else
    return int(send(fd,data,size,0));
#endif
}
bool wouldBlock() { return errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR; }
bool nonblocking(int fd) {
#ifdef SO_NOSIGPIPE
    const int on = 1; setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&on,sizeof(on));
#endif
    const int flags = fcntl(fd,F_GETFL,0);
    return flags >= 0 && fcntl(fd,F_SETFL,flags|O_NONBLOCK) >= 0;
}
#endif
}
struct PhotoShare::Impl {
    std::thread worker;
    std::atomic<bool> stopping{true};
    int listener = -1;
    std::string directory, address, prefix, url;
    std::vector<PhotoEntry> photos;
    std::set<std::string> allowed;
#ifdef __vita__
    std::vector<uint8_t> netMemory;
    bool moduleLoaded = false, netReady = false, ctlReady = false;
#endif
    bool connected() const {
#ifdef __vita__
        int state = 0;
        SceNetCtlInfo info{};
        return ctlReady && sceNetCtlInetGetState(&state) >= 0 && state == SCE_NETCTL_STATE_CONNECTED
            && sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS,&info) >= 0 && address == info.ip_address;
#else
        return !stopping;
#endif
    }
    void stop() {
        stopping = true;
        if (worker.joinable()) worker.join();
        closeSocket(listener); listener = -1;
#ifdef __vita__
        if (ctlReady) sceNetCtlTerm();
        if (netReady) sceNetTerm();
        if (moduleLoaded) sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
        ctlReady = netReady = moduleLoaded = false;
        netMemory.clear();
#endif
        url.clear();
    }
    bool setup(std::string& error) {
#ifdef __vita__
        if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) != SCE_SYSMODULE_LOADED) {
            const int result = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
            logResult("share: network module",result);
            if (result < 0) { error = "Wi-Fi could not start"; return false; }
            moduleLoaded = true;
        }
        netMemory.resize(1024*1024);
        SceNetInitParam params{};
        params.memory = netMemory.data(); params.size = netMemory.size();
        int result = sceNetInit(&params);
        logResult("share: network init",result);
        if (result < 0) { error = "Network is unavailable"; return false; }
        netReady = true;
        result = sceNetCtlInit();
        logResult("share: network control",result);
        if (result < 0) { error = "Wi-Fi is unavailable"; return false; }
        ctlReady = true;
        SceNetCtlInfo info{};
        if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS,&info) < 0) {
            error = "Connect the Vita to Wi-Fi, then retry"; return false;
        }
        address = info.ip_address;
        if (!connected()) { error = "Connect the Vita to Wi-Fi, then retry"; return false; }
#else
        address = "127.0.0.1";
#endif
        uint8_t random[16];
#ifdef __vita__
        if (sceKernelGetRandomNumber(random,sizeof(random)) < 0) { error = "Could not create a sharing session"; return false; }
#else
        std::random_device source;
        for (auto& byte : random) byte = uint8_t(source());
#endif
        const char* hex = "0123456789abcdef";
        prefix = "/";
        for (auto byte : random) { prefix += hex[byte>>4]; prefix += hex[byte&15]; }
        prefix += '/';
        Address bindAddress{};
#ifdef __vita__
        listener = sceNetSocket("VitaCameraShare",SCE_NET_AF_INET,SCE_NET_SOCK_STREAM,0);
        bindAddress.sin_len = sizeof(bindAddress); bindAddress.sin_family = SCE_NET_AF_INET;
        sceNetInetPton(SCE_NET_AF_INET,address.c_str(),&bindAddress.sin_addr);
        unsigned length = sizeof(bindAddress);
        const bool bound = listener >= 0 && sceNetBind(listener,reinterpret_cast<SceNetSockaddr*>(&bindAddress),length) >= 0
            && sceNetListen(listener,16) >= 0 && sceNetGetsockname(listener,reinterpret_cast<SceNetSockaddr*>(&bindAddress),&length) >= 0;
        const unsigned port = sceNetNtohs(bindAddress.sin_port);
#else
        listener = socket(AF_INET,SOCK_STREAM,0);
        bindAddress.sin_family = AF_INET;
        inet_pton(AF_INET,address.c_str(),&bindAddress.sin_addr);
        socklen_t length = sizeof(bindAddress);
        const bool bound = listener >= 0 && bind(listener,reinterpret_cast<sockaddr*>(&bindAddress),length) >= 0
            && listen(listener,16) >= 0 && getsockname(listener,reinterpret_cast<sockaddr*>(&bindAddress),&length) >= 0;
        const unsigned port = ntohs(bindAddress.sin_port);
#endif
        if (!bound || !nonblocking(listener)) { error = "Could not open photo sharing"; return false; }
        url = "http://"+address+":"+std::to_string(port)+prefix;
        return true;
    }
    bool send(int client,const char* bytes,size_t length) {
        auto deadline = Clock::now()+std::chrono::seconds(5);
        while (length && !stopping) {
            const int sent = transmit(client,bytes,length);
            if (sent > 0) { bytes += sent; length -= size_t(sent); deadline = Clock::now()+std::chrono::seconds(5); }
            else if (sent == 0 || !wouldBlock() || Clock::now() >= deadline) return false;
            else pauseSocket();
        }
        return !stopping;
    }
    bool headers(int client,int status,const std::string& type,size_t length,const std::string& filename = "") {
        std::ostringstream out;
        out << "HTTP/1.1 " << status << (status == 200 ? " OK" : status == 404 ? " Not Found" : status == 405 ? " Method Not Allowed" : " Bad Request")
            << "\r\nContent-Type: " << type << "\r\nContent-Length: " << length
            << "\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer"
            << "\r\nContent-Security-Policy: default-src 'none'; img-src 'self'; style-src 'unsafe-inline'; base-uri 'none'; frame-ancestors 'none'\r\n";
        if (status == 405) out << "Allow: GET, HEAD\r\n";
        if (!filename.empty()) out << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n";
        out << "\r\n";
        const auto message = out.str();
        return send(client,message.data(),message.size());
    }
    void serve(int client) {
        std::string request;
        char buffer[16384];
        const auto deadline = Clock::now()+std::chrono::seconds(3);
        while (!stopping && request.find("\r\n\r\n") == std::string::npos) {
            if (request.size() >= 8192) { headers(client,400,"text/plain",0); return; }
            const int got = receive(client,buffer,std::min(size_t(1024),8192-request.size()));
            if (got > 0) request.append(buffer,size_t(got));
            else if (!got || !wouldBlock() || Clock::now() >= deadline) return;
            else pauseSocket();
        }
        std::istringstream line(request.substr(0,request.find("\r\n")));
        std::string method,target,version,extra;
        if (!(line >> method >> target >> version) || (line >> extra) || (version != "HTTP/1.1" && version != "HTTP/1.0")) {
            headers(client,400,"text/plain",0); return;
        }
        if (method != "GET" && method != "HEAD") { headers(client,405,"text/plain",0); return; }
        if (target.rfind(prefix,0) != 0) { headers(client,404,"text/plain",0); return; }
        const size_t queryAt = target.find('?');
        const std::string query = queryAt == std::string::npos ? "" : target.substr(queryAt+1);
        const std::string route = target.substr(0,queryAt).substr(prefix.size());
        if (route.empty()) {
            size_t page = 0;
            if (!query.empty()) {
                if (query.rfind("page=",0) != 0 || query.size() < 6 || query.size() > 11 || query.substr(5).find_first_not_of("0123456789") != std::string::npos) {
                    headers(client,400,"text/plain",0); return;
                }
                for (size_t i = 5; i < query.size(); ++i) page = page*10+query[i]-'0';
            }
            const auto html = sharingPage(photos,page);
            if (headers(client,200,"text/html; charset=utf-8",html.size()) && method == "GET") send(client,html.data(),html.size());
            return;
        }
        const std::string name = route.rfind("photo/",0) == 0 ? route.substr(6) : "";
        if (!isPhotoName(name) || !allowed.count(name) || (!query.empty() && query != "download=1")) {
            headers(client,404,"text/plain",0); return;
        }
        FILE* file = std::fopen((directory+"/"+name).c_str(),"rb");
        if (!file) { headers(client,404,"text/plain",0); return; }
        if (std::fseek(file,0,SEEK_END) != 0) { std::fclose(file); headers(client,404,"text/plain",0); return; }
        const long length = std::ftell(file);
        std::rewind(file);
        if (length < 0) { std::fclose(file); headers(client,404,"text/plain",0); return; }
        if (headers(client,200,"image/png",size_t(length),query.empty() ? "" : name) && method == "GET") {
            size_t remaining = size_t(length);
            while (remaining && !stopping) {
                const size_t got = std::fread(buffer,1,std::min(remaining,sizeof(buffer)),file);
                if (!got || !send(client,buffer,got)) break;
                remaining -= got;
            }
        }
        std::fclose(file);
    }
    void work() {
        while (!stopping) {
            const int client = acceptSocket(listener);
            if (client < 0) { pauseSocket(); continue; }
            if (nonblocking(client)) serve(client);
            closeSocket(client);
        }
    }
};
PhotoShare::PhotoShare() : impl_(std::make_unique<Impl>()) {}
PhotoShare::~PhotoShare() { stop(); }
void PhotoShare::stop() { impl_->stop(); }
bool PhotoShare::active() const { return !impl_->stopping; }
bool PhotoShare::connected() const { return active() && impl_->connected(); }
const std::string& PhotoShare::url() const { return impl_->url; }
bool PhotoShare::start(const std::string& directory,std::string& error) {
    stop(); error.clear();
    impl_->directory = directory; impl_->photos = scanPhotos(directory); impl_->allowed.clear();
    for (const auto& photo : impl_->photos) { impl_->allowed.insert(photo.original); impl_->allowed.insert(photo.image); }
    try {
        if (!impl_->setup(error)) { stop(); return false; }
        impl_->stopping = false;
        impl_->worker = std::thread(&Impl::work,impl_.get());
    } catch (const std::exception&) { error = "Could not start photo sharing"; stop(); return false; }
    return true;
}
}
