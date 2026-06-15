#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <thread>

#include "shared_types.hpp"

int main() {
    // ── attach to shared memory created by writer ─────────────────────
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd < 0) { perror("shm_open (run writer first)"); return 1; }

    auto* shm = static_cast<SharedRegion*>(
        mmap(nullptr, sizeof(SharedRegion), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    // ── open semaphores created by writer ─────────────────────────────
    sem_t* mutex = sem_open(SEM_MUTEX, 0);
    sem_t* full  = sem_open(SEM_FULL,  0);
    sem_t* empty = sem_open(SEM_EMPTY, 0);

    printf("[reader] attached to shm, consuming %d frames\n\n", TOTAL_FRAMES);

    for (int i = 0; i < TOTAL_FRAMES; ++i) {
        sem_wait(full);    // block if ring buffer is empty
        sem_wait(mutex);   // lock

        LidarFrame frame = shm->frames[shm->tail];
        shm->tail = (shm->tail + 1) % CAPACITY;
        printf("[reader] consumed frame %2d  dist=%.1fm\n", frame.id, frame.distance_m);

        sem_post(mutex);   // unlock
        sem_post(empty);   // wake writer

        // Simulate processing — slightly faster than writer
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // ── cleanup: reader is responsible for final teardown ─────────────
    sem_close(mutex); sem_unlink(SEM_MUTEX);
    sem_close(full);  sem_unlink(SEM_FULL);
    sem_close(empty); sem_unlink(SEM_EMPTY);
    munmap(shm, sizeof(SharedRegion));
    shm_unlink(SHM_NAME);

    printf("\n[reader] done, resources cleaned up\n");
}
