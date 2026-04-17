#include <cassert>
#include <iostream>
#include <unordered_map>

#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

struct ClientInfo {
    int fd;
    time_t lastActive;
    std::string ip;
    int port;
};

void setnonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void insertfd(int fd, int epfd) {
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void closeClient(int epfd, std::unordered_map<int, ClientInfo>& clients, int fd, const char* reason) {
    auto it = clients.find(fd);
    if (it != clients.end()) {
        time_t now = time(nullptr);
        std::cout << "[LOG] close connection, fd=" << it->second.fd
                  << ", ip=" << it->second.ip
                  << ", port=" << it->second.port
                  << ", reason=" << reason
                  << ", idle=" << (now - it->second.lastActive) << "s"
                  << std::endl;

        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        clients.erase(it);
    }
}

void timelimitex(int epfd, std::unordered_map<int, ClientInfo>& clients) {
    time_t now = time(nullptr);
    for (auto it = clients.begin(); it != clients.end();) {
        if (now - it->second.lastActive > 30) {
            int fd = it->first;
            ++it;
            closeClient(epfd, clients, fd, "timeout");
        } else {
            ++it;
        }
    }
}

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setnonblocking(listenfd);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    assert(bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1);
    assert(listen(listenfd, 128) != -1);

    std::cout << "Server is listening on port 8080..." << std::endl;

    int epfd = epoll_create1(0);
    assert(epfd >= 0);

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
    ev.data.fd = listenfd;
    assert(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) != -1);

    epoll_event events[1024];
    std::unordered_map<int, ClientInfo> clients;

    while (true) {
        int n = epoll_wait(epfd, events, 1024, 1000);
        if (n < 0) {
            std::cerr << "epoll_wait failed\n";
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listenfd) {
                while (true) {
                    sockaddr_in clientAddr{};
                    socklen_t len = sizeof(clientAddr);
                    int clientfd = accept(listenfd, reinterpret_cast<sockaddr*>(&clientAddr), &len);
                    if (clientfd < 0) {
                        break;
                    }

                    insertfd(clientfd, epfd);

                    char ip[INET_ADDRSTRLEN] = {0};
                    inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));

                    clients[clientfd] = {clientfd, time(nullptr), ip, ntohs(clientAddr.sin_port)};

                    std::cout << "New connection from " << ip << ":" << ntohs(clientAddr.sin_port) << std::endl;
                }
            } else {
                if (events[i].events & EPOLLIN) {
                    char buffer[1024];
                    int clientfd = events[i].data.fd;
                    int n = recv(clientfd, buffer, sizeof(buffer) - 1, 0);

                    if (n > 0) {
                        buffer[n] = '\0';
                        std::cout << "Received from client: " << buffer << std::endl;
                        std::string cur = buffer;
                        cur = "client [" + std::to_string(clientfd) + "]: " + cur + "\n";

                        for (auto& c : clients) {
                            if (c.first != clientfd) {
                                send(c.first, cur.c_str(), cur.size(), 0);
                            }
                        }

                        clients[clientfd].lastActive = time(nullptr);
                    } else if (n == 0) {
                        closeClient(epfd, clients, clientfd, "client closed");
                    } else {
                        closeClient(epfd, clients, clientfd, "recv error");
                    }
                } else if (events[i].events & (EPOLLHUP | EPOLLERR)) {
                    closeClient(epfd, clients, events[i].data.fd, "client disconnected");
                }
            }
        }

        timelimitex(epfd, clients);
    }
}