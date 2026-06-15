#pragma once

// Names for POSIX shared memory and semaphores.
// Semaphores in shared memory (sem_init pshared=1) are not supported on macOS,
// so we use named semaphores (sem_open) which work on both Linux and macOS.

static constexpr int  CAPACITY     = 4;    // ring buffer slots
static constexpr int  TOTAL_FRAMES = 12;   // frames writer will produce

static constexpr char SHM_NAME[]   = "/lidar_shm";
static constexpr char SEM_MUTEX[]  = "/lidar_mutex";  // binary sem, guards ring buffer
static constexpr char SEM_FULL[]   = "/lidar_full";   // counts items available to reader
static constexpr char SEM_EMPTY[]  = "/lidar_empty";  // counts free slots for writer

// Only POD types can live in shared memory — no vtables, no heap pointers.
struct LidarFrame {
    int   id;
    float distance_m;
};

struct SharedRegion {
    LidarFrame frames[CAPACITY];
    int        head;   // writer inserts here
    int        tail;   // reader removes here
};
