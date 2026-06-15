#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

static constexpr int         PORT     = 9092;
static constexpr const char* HOST     = "127.0.0.1";
static constexpr int         BUF_SIZE = 1024;

static std::atomic<bool> g_running{true};

static void recv_loop(int sock) {
    char buf[BUF_SIZE];
    while (g_running) {
        int n = recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) {
            std::cout << "[server closed]\n";
            g_running = false;
            break;
        }
        std::cout << std::string(buf, n) << std::flush;
    }
}

int main(int argc, char* argv[]) {
    std::string name = (argc > 1) ? argv[1] : "anonymous";

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port   = htons(PORT);
    inet_pton(AF_INET, HOST, &server.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0) {
        perror("connect"); return 1;
    }
    std::cout << "[*] connected as " << name << "\n";

    std::thread recv_thread(recv_loop, sock);

    std::string line;
    while (g_running && std::getline(std::cin, line)) {
        std::string msg = name + ": " + line + "\n";
        send(sock, msg.data(), msg.size(), 0);
    }

    g_running = false;
    shutdown(sock, SHUT_WR);
    recv_thread.join();
    close(sock);
}
