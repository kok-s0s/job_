#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "blocking_queue.hpp"

struct LidarFrame {
    int    id;
    int    producer_id;
    float  distance_m;
};

static constexpr int    QUEUE_CAPACITY  = 4;
static constexpr int    NUM_PRODUCERS   = 2;
static constexpr int    NUM_CONSUMERS   = 2;
static constexpr int    FRAMES_PER_PRODUCER = 8;

static std::mutex       g_cout_mtx;
static std::atomic<int> g_produced{0};
static std::atomic<int> g_consumed{0};

static void log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_cout_mtx);
    std::cout << msg << "\n";
}

// Sensor thread: generate LiDAR frames at ~10 Hz
static void producer(BlockingQueue<LidarFrame>& q, int pid) {
    for (int i = 0; i < FRAMES_PER_PRODUCER; ++i) {
        LidarFrame frame{
            .id          = pid * 100 + i,
            .producer_id = pid,
            .distance_m  = 1.0f + i * 0.1f,
        };
        if (!q.push(frame)) break;   // queue shut down

        int total = ++g_produced;
        log("  [producer " + std::to_string(pid) + "] push frame " +
            std::to_string(frame.id) +
            "  (queue=" + std::to_string(q.size()) +
            "/" + std::to_string(q.capacity()) + ")"
            "  total_produced=" + std::to_string(total));

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    log("[producer " + std::to_string(pid) + "] done");
}

// Processing thread: consume and process frames
static void consumer(BlockingQueue<LidarFrame>& q, int cid) {
    LidarFrame frame;
    while (q.pop(frame)) {
        int total = ++g_consumed;
        log("  [consumer " + std::to_string(cid) + "] processed frame " +
            std::to_string(frame.id) +
            "  dist=" + std::to_string(frame.distance_m).substr(0, 4) + "m" +
            "  total_consumed=" + std::to_string(total));

        // Consumers are slower → demonstrates queue backpressure
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }
    log("[consumer " + std::to_string(cid) + "] done");
}

int main() {
    BlockingQueue<LidarFrame> q(QUEUE_CAPACITY);

    std::cout << "queue capacity=" << QUEUE_CAPACITY
              << "  producers=" << NUM_PRODUCERS
              << "  consumers=" << NUM_CONSUMERS << "\n\n";

    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CONSUMERS; ++i)
        threads.emplace_back(consumer, std::ref(q), i);

    for (int i = 0; i < NUM_PRODUCERS; ++i)
        threads.emplace_back(producer, std::ref(q), i);

    // Wait for all producers to finish, then shut down the queue.
    // Consumers will drain remaining items before exiting.
    for (int i = NUM_CONSUMERS; i < (int)threads.size(); ++i)
        threads[i].join();

    q.shutdown();

    for (int i = 0; i < NUM_CONSUMERS; ++i)
        threads[i].join();

    std::cout << "\nproduced=" << g_produced
              << "  consumed=" << g_consumed << "\n";
}
