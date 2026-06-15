#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "shared_types.hpp"

int main() {
    // ── shared memory ────────────────────────────────────────────────────
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd < 0) { perror("shm_open"); return 1; }
    ftruncate(fd, sizeof(SharedRegion));

    auto* shm = static_cast<SharedRegion*>(
        mmap(nullptr, sizeof(SharedRegion), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    std::memset(shm, 0, sizeof(SharedRegion));  // zero head/tail

    // ── named semaphores ─────────────────────────────────────────────────
    // Unlink first to clear any leftover state from a previous crashed run.
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_EMPTY);

    sem_t* mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);         // binary
    sem_t* full  = sem_open(SEM_FULL,  O_CREAT, 0666, 0);         // starts empty
    sem_t* empty = sem_open(SEM_EMPTY, O_CREAT, 0666, CAPACITY);  // all slots free

    printf("[writer] shm ready, producing %d frames (capacity=%d)\n\n",
           TOTAL_FRAMES, CAPACITY);

    for (int i = 0; i < TOTAL_FRAMES; ++i) {
        sem_wait(empty);   // block if ring buffer is full
        sem_wait(mutex);   // lock

        shm->frames[shm->head] = LidarFrame{i, 1.0f + i * 0.1f};
        shm->head = (shm->head + 1) % CAPACITY;
        printf("[writer] pushed  frame %2d  dist=%.1fm\n", i, 1.0f + i * 0.1f);

        sem_post(mutex);   // unlock
        sem_post(full);    // wake reader

        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }

    sem_close(mutex);
    sem_close(full);
    sem_close(empty);
    munmap(shm, sizeof(SharedRegion));

    printf("\n[writer] done\n");
}
