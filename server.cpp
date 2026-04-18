#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/util.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <ctime>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <string>

using namespace std;

static event_base* base = nullptr;
static unordered_map<int, bufferevent*> clients;
static unordered_map<int, time_t> lastActive;
static event* timer_ev = nullptr;

const int TIMELIMIT = 60;
const int CHECK_INTERVAL = 5;

void broadcast_msg(const string& msg, int skip_fd = -1) {
    for (auto& [fd, bev] : clients) {
        if (fd == skip_fd) continue;
        bufferevent_write(bev, msg.c_str(), msg.size());
    }
}

void read_cb(bufferevent* bev, void* arg) {
    char buf[1024];
    int fd = bufferevent_getfd(bev);
    while (1) {
        memset(buf, 0, sizeof(buf));
        int n = bufferevent_read(bev, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        lastActive[fd] = time(nullptr);

        string msg = "client " + to_string(fd) + ": " + string(buf);
        cout << msg;
        broadcast_msg(msg, fd);
    }
}

void event_cb(bufferevent* bev, short events, void* arg) {
    int fd = bufferevent_getfd(bev);

    if (events & BEV_EVENT_EOF) {
        cout << "client[" << fd << "] disconnected." << endl;
    } else if (events & BEV_EVENT_ERROR) {
        cout << "client[" << fd << "] error." << endl;
    } else {
        return;
    }

    string msg = "client " + to_string(fd) + " left.\n";

    clients.erase(fd);
    lastActive.erase(fd);
    bufferevent_free(bev);

    broadcast_msg(msg, fd);
}

void accept_cb(evconnlistener* listener, evutil_socket_t fd,
               sockaddr* addr, int socklen, void* ctx) {
    bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (!bev) {
        close(fd);
        return;
    }

    bufferevent_setcb(bev, read_cb, nullptr, event_cb, nullptr);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    clients[fd] = bev;
    lastActive[fd] = time(nullptr);

    cout << "client[" << fd << "] connected." << endl;

    string msg = "client " + to_string(fd) + " joined.\n";
    broadcast_msg(msg, fd);
}

void timer_cb(evutil_socket_t /*fd*/, short /*events*/, void* /*arg*/) {
    time_t now = time(nullptr);
    for(auto it = lastActive.begin(); it != lastActive.end();) {
        if(now - it->second > TIMELIMIT) {
            int fd = it->first;
            cout << "client[" << fd << "] timed out." << endl;

            string msg = "client " + to_string(fd) + " timed out.\n";
            broadcast_msg(msg, fd);

            bufferevent_free(clients[fd]);
            clients.erase(fd);
            it = lastActive.erase(it);
        } else {
            ++it;
        }
    }

    timeval tv{CHECK_INTERVAL, 0};
    evtimer_add(timer_ev, &tv);
}

int main() {
    base = event_base_new();
    if (!base) {
        cout << "event_base_new failed." << endl;
        return 1;
    }

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(8888);

    evconnlistener* listener = evconnlistener_new_bind(
        base,
        accept_cb,
        nullptr,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
        -1,
        (sockaddr*)&sin,
        sizeof(sin)
    );

    if (!listener) {
        cout << "evconnlistener_new_bind failed." << endl;
        event_base_free(base);
        return 1;
    }

    timer_ev = evtimer_new(base, timer_cb, nullptr);
    if (!timer_ev) {
        cout << "evtimer_new failed." << endl;
        evconnlistener_free(listener);
        event_base_free(base);
        return 1;
    }
    timeval tv{CHECK_INTERVAL, 0};
    evtimer_add(timer_ev, &tv);
    cout << "server started on port 8888." << endl;
    event_base_dispatch(base);

    event_free(timer_ev);
    evconnlistener_free(listener);
    event_base_free(base);
    return 0;
}
