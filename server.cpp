#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <unordered_set>
#include <string>

void set_nonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    if(old_option < 0) {
        std::cerr << "fcntl getfl error" << std::endl;
        return;
    }

    int new_option = old_option | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, new_option) < 0) {
        std::cerr << "fcntl setfl error" << std::endl;
    }
}

void add_fd(int epoll_fd, int fd) {
    epoll_event ev{};
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLRDHUP;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "epoll_ctl add error" << std::endl;
        return;
    }
    set_nonblocking(fd);
}

void remove_fd(int epoll_fd, int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

int main(int argc, const char* argv[]) {
    if(argc <= 2) {
        std::cerr << "need ip port" << std::endl;
        return 1;
    }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    std::string ip = argv[1];
    int port = atoi(argv[2]);

    if(listenfd < 0) {
        std::cerr << "socket error" << std::endl;
        return 1;
    }

    int reuse = 1;
    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "setsockopt error" << std::endl;
        close(listenfd);
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "ip error" << std::endl;
        close(listenfd);
        return 1;
    }

    if(bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind error" << std::endl;
        close(listenfd);
        return 1;
    }

    if(listen(listenfd, 8) < 0) {
        std::cerr << "listen error" << std::endl;
        close(listenfd);
        return 1;
    }

    int epollfd = epoll_create1(0);
    if(epollfd < 0) {
        std::cerr << "epoll_create1 error" << std::endl;
        close(listenfd);
        return 1;
    }

    add_fd(epollfd, listenfd);
    epoll_event events[1024];
    std::unordered_set<int> clients;

    while(1) {
        int num = epoll_wait(epollfd, events, 1024, -1);
        if(num < 0) {
            if(errno == EINTR) continue;
            std::cerr << "epoll_wait error" << std::endl;
            break;
        }

        for(int i = 0; i < num; ++i) {
            int sockfd = events[i].data.fd;
            auto ev = events[i].events;

            if(sockfd == listenfd) {
                while(1) {
                    sockaddr_in client_addr{};
                    socklen_t client_len = sizeof(client_addr);
                    int confd = accept(listenfd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

                    if(confd < 0) {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        std::cerr << "accept error" << std::endl;
                        break;
                    }

                    if(clients.size() >= 4096) {
                        std::string msg = "too many user\n";
                        send(confd, msg.c_str(), msg.size(), 0);
                        close(confd);
                        continue;
                    }

                    add_fd(epollfd, confd);
                    clients.insert(confd);

                    char client_ip[1024]{};
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip) - 1);

                    std::cout << "new user: " << client_ip
                            << ": " << ntohs(client_addr.sin_port)
                            << ", fd = " << confd << std::endl;
                }
            } else if(ev & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                std::cout << "client fd " << sockfd << " disconnected" << std::endl;
                clients.erase(sockfd);
                remove_fd(epollfd, sockfd);
            } else if(ev & EPOLLIN) {
                char buf[1024];
                while(1) {
                    std::fill(buf, buf + 1024, 0);
                    int ret = recv(sockfd, buf, sizeof(buf) - 1, 0);

                    if(ret > 0) {
                        std::string msg = "client[" + std::to_string(sockfd) + "]: " + std::string(buf, ret);
                        std::cout << msg;
                        
                        for(int fd : clients) {
                            if(fd == sockfd) continue;
                            int sret = send(fd, msg.c_str(), msg.size(), 0);
                            if(sret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                                std::cerr << "send to fd " << fd << " failed\n";
                            }
                        }
                    } else if(ret == 0) {
                        std::cout << "client fd " << sockfd << " closed\n";
                        clients.erase(sockfd);
                        remove_fd(epollfd, sockfd);
                        break;
                    } else {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        std::cerr << "recv error" << std::endl;
                        clients.erase(sockfd);
                        remove_fd(epollfd, sockfd);
                        break;
                    }
                }
            }
        }
    }

    close(listenfd);
    close(epollfd);
    return 0;
}