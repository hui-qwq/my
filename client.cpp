#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

void set_nonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    if(old_option < 0) {
        cerr << "fcntl getfl error" << endl;
        return;
    }

    int new_option = old_option | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, new_option) < 0) {
        cerr << "fcntl setfl error" << endl;
    }
}

void add_fd(int epoll_fd, int fd) {
    epoll_event ev{};
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLRDHUP;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        cerr << "epoll_ctl add error" << endl;
        return;
    }
    set_nonblocking(fd);
}

void remove_fd(int epoll_fd, int fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

int pipefd[2];
int sockfd, epollfd;

void let_out() {
    close(pipefd[0]);
    close(pipefd[1]);
    close(sockfd);
    close(epollfd);
}

int main(int argc, char* argv[]) {
    if(argc <= 2) {
        cerr << "ip port" << endl;
        return 1;
    }

    string ip = argv[1];
    int port = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        cerr << "sockfd error" << endl;
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if(connect(sockfd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        cerr << "connect" << endl;
        close(sockfd);
        return 1;
    }

    if(pipe(pipefd) < 0) {
        cerr << "pipe error" << endl;
        close(sockfd);
        return 1;
    }

    epollfd = epoll_create1(0);
    if(epollfd < 0) {
        cerr << "epoll_create1 error" << endl;
        close(pipefd[0]);
        close(pipefd[1]);
        close(sockfd);
        return 1;
    }

    add_fd(epollfd, 0);
    add_fd(epollfd, sockfd);

    epoll_event events[1024];

    while(1) {
        int num = epoll_wait(epollfd, events, 1024, -1);
        if(num < 0) {
            if(errno == EINTR) continue;
            cerr << "epoll_wait" << endl;
            break;
        }

        for(int i = 0; i < num; ++i) {
            int fd = events[i].data.fd;
            auto ev = events[i].events;

            if(fd == sockfd) {
                if(ev & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
                    cout << "server close connection" << endl;
                    let_out();
                    return 0;
                }

                if(ev & EPOLLIN) {
                    char buf[1024];
                    while(1) {
                        fill(buf, buf + 1024, 0);
                        int ret = recv(sockfd, buf, sizeof(buf) - 1, 0);
                        if(ret > 0) {
                            cout.write(buf, ret);
                            cout.flush();
                        } else if(ret == 0) {
                            cout << "server close connection" << endl;
                            let_out();
                            return 0;
                        } else {
                            if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                            cerr << "recv error" << endl;
                            let_out();
                            return 0;
                        }
                    }
                }
            }

            if(fd == 0 && (ev & EPOLLIN)) {
                while(1) {
                    int ret = splice(0, nullptr, pipefd[1], nullptr, 32768,
                                    SPLICE_F_MORE | SPLICE_F_MOVE);
                    if(ret > 0) {
                        int sent = 0;
                        while(sent < ret) {
                            int n = splice(pipefd[0], nullptr, sockfd, nullptr, ret - sent,
                                        SPLICE_F_MORE | SPLICE_F_MOVE);
                            if(n > 0) {
                                sent += n;
                            } else {
                                if(errno == EAGAIN || errno == EWOULDBLOCK) continue;
                                cerr << "pipe->socket error" << endl;
                                let_out();
                                return 1;
                            }
                        }
                    } else if(ret == 0) {
                        let_out();
                        return 1;
                    } else {
                        if(errno == EAGAIN || errno == EWOULDBLOCK) break;
                        cerr << "stdin->pipe error" << endl;
                        let_out();
                        return 1;
                    }
                }
            }
        }
    }

    let_out();
    return 0;
}