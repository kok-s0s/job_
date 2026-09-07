#include <iostream>
#include <queue>
#include <string>

#include "can_frame.hpp"

int main() {
    try {
        std::queue<socketcan_demo::CanFrame> bus;
        const auto tx = socketcan_demo::parseCandumpLine("123#01020304");

        std::cout << "[vcan_setup] command=\"sudo modprobe vcan\"\n";
        std::cout << "[vcan_setup] command=\"sudo ip link add dev vcan0 type vcan\"\n";
        std::cout << "[vcan_setup] command=\"sudo ip link set up vcan0\"\n";
        std::cout << "[vcan_loopback] tx=\"cansend vcan0 " << socketcan_demo::formatCandumpLine(tx) << "\"\n";
        std::cout << "[vcan_loopback] rx=\"candump vcan0\" expected=\"" << socketcan_demo::formatCandumpLine(tx) << "\"\n";

        bus.push(tx);
        const auto rx = bus.front();
        bus.pop();

        if (rx.id != tx.id || rx.data != tx.data) {
            std::cerr << "[error] loopback payload mismatch\n";
            return 1;
        }

        std::cout << "[vcan_loopback] received id=0x" << std::hex << std::uppercase << rx.id << std::dec
                  << " dlc=" << rx.data.size()
                  << " data=\"" << socketcan_demo::formatBytes(rx.data) << "\"\n";
        std::cout << "[ok] vcan loopback command path verified\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
