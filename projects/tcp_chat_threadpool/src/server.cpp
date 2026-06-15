#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "thread_pool.hpp"

static constexpr int PORT        = 9091;
static constexpr int BACKLOG     = 10;
static constexpr int BUF_SIZE    = 1024;
static constexpr int NUM_WORKERS = 4;   // fixed thread count

static std::unordered_map<int, std::string> g_clients;
static std::mutex                            g_clients_mtx;
static std::mutex                            g_cout_mtx;

static void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_cout_mtx);
    std::cout << msg << "\n";
}

static void broadcast(int sender_fd, const std::string& msg) {
    std::vector<int> recipients;
    {
        std::lock_guard<std::mutex> lock(g_clients_mtx);
        for (auto& [fd, _] : g_clients)
            if (fd != sender_fd) recipients.push_back(fd);
    }
    for (int fd : recipients)
        send(fd, msg.data(), msg.size(), 0);
}

static void handle_client(int client_fd, std::string peer) {
    {
        std::lock_guard<std::mutex> lock(g_clients_mtx);
        g_clients[client_fd] = peer;
    }
    const std::string join_msg = "*** " + peer + " joined ***\n";
    log(join_msg);
    broadcast(client_fd, join_msg);

    char buf[BUF_SIZE];
    while (true) {
        int n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) break;
        std::string msg = "[" + peer + "] " + std::string(buf, n);
        log(msg);
        broadcast(client_fd, msg);
    }

    {
        std::lock_guard<std::mutex> lock(g_clients_mtx);
        g_clients.erase(client_fd);
    }
    const std::string leave_msg = "*** " + peer + " left ***\n";
    log(leave_msg);
    broadcast(client_fd, leave_msg);
    close(client_fd);
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(server_fd, BACKLOG);
    log("[*] chat server (thread pool, " + std::to_string(NUM_WORKERS) +
        " workers) on port " + std::to_string(PORT));

    ThreadPool pool(NUM_WORKERS);

    while (true) {
        sockaddr_in peer_addr{};
        socklen_t   peer_len = sizeof(peer_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_addr.sin_addr, ip, sizeof(ip));
        std::string peer = std::string(ip) + ":" + std::to_string(ntohs(peer_addr.sin_port));

        // submit to pool instead of spawning a new thread per connection
        pool.submit([client_fd, peer] { handle_client(client_fd, peer); });
    }

    close(server_fd);
}
