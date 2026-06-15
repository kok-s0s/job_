#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

static constexpr int PORT     = 8080;
static constexpr int BACKLOG  = 10;
static constexpr int BUF_SIZE = 1024;

static std::mutex g_cout_mtx;

static void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_cout_mtx);
    std::cout << msg << "\n";
}

static void handle_client(int client_fd, std::string peer) {
    log("[+] connected: " + peer);

    char buf[BUF_SIZE];
    while (true) {
        int n = recv(client_fd, buf, sizeof(buf), 0);
        if (n <= 0) break;

        log("[" + peer + "] " + std::string(buf, n));

        // send_full: recv/send may transfer fewer bytes than requested
        int sent = 0;
        while (sent < n) {
            int r = send(client_fd, buf + sent, n - sent, 0);
            if (r <= 0) goto done;
            sent += r;
        }
    }
done:
    log("[-] disconnected: " + peer);
    close(client_fd);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    // avoid "Address already in use" on quick restart
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
    log("[*] listening on port " + std::to_string(PORT));

    while (true) {
        sockaddr_in peer_addr{};
        socklen_t peer_len = sizeof(peer_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
        if (client_fd < 0) { perror("accept"); continue; }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_addr.sin_addr, ip, sizeof(ip));
        std::string peer = std::string(ip) + ":" + std::to_string(ntohs(peer_addr.sin_port));

        // detach: client thread owns its own lifetime, no need to join
        std::thread(handle_client, client_fd, peer).detach();
    }

    close(server_fd);
}
