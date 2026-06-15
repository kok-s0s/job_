#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "poller.hpp"

static constexpr int PORT     = 9092;
static constexpr int BACKLOG  = 10;
static constexpr int BUF_SIZE = 1024;

// No mutex: the entire event loop runs on a single thread.
// g_clients is only ever touched here, no concurrent access possible.
static std::unordered_map<int, std::string> g_clients;

static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void broadcast(int skip_fd, const std::string& msg) {
    for (auto& [fd, _] : g_clients)
        if (fd != skip_fd)
            send(fd, msg.data(), msg.size(), 0);
}

static void on_connect(int server_fd, Poller& poller) {
    sockaddr_in peer_addr{};
    socklen_t   peer_len = sizeof(peer_addr);
    int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
    if (client_fd < 0) return;

    set_nonblocking(client_fd);
    poller.add(client_fd);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer_addr.sin_addr, ip, sizeof(ip));
    std::string peer = std::string(ip) + ":" + std::to_string(ntohs(peer_addr.sin_port));
    g_clients[client_fd] = peer;

    std::string msg = "*** " + peer + " joined ***\n";
    std::cout << msg;
    broadcast(client_fd, msg);
}

static void on_readable(int fd, Poller& poller) {
    char buf[BUF_SIZE];
    int  n = recv(fd, buf, sizeof(buf), 0);

    if (n <= 0) {
        // n == 0: clean close.  n < 0 && errno == EAGAIN: no data yet (non-blocking).
        // Treat both disconnect cases the same: remove and notify.
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;

        std::string peer = g_clients[fd];
        poller.remove(fd);
        g_clients.erase(fd);
        close(fd);

        // fd is gone from g_clients, so broadcast(-1, ...) sends to all remaining
        std::string msg = "*** " + peer + " left ***\n";
        std::cout << msg;
        broadcast(-1, msg);
        return;
    }

    std::string peer = g_clients[fd];
    std::string msg  = "[" + peer + "] " + std::string(buf, n);
    std::cout << msg;
    broadcast(fd, msg);
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(server_fd, BACKLOG);

    Poller poller;
    poller.add(server_fd);

#ifdef __linux__
    std::cout << "[*] epoll chat server on port " << PORT << "\n";
#else
    std::cout << "[*] kqueue chat server on port " << PORT << "\n";
#endif

    std::vector<int> ready;
    while (true) {
        poller.wait(ready);
        for (int fd : ready) {
            if (fd == server_fd)
                on_connect(server_fd, poller);
            else
                on_readable(fd, poller);
        }
    }

    close(server_fd);
}
