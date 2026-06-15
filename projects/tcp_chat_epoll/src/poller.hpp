#pragma once

// Thin abstraction over epoll (Linux) and kqueue (macOS/BSD).
// Both follow the same pattern: register fds, wait for readable events,
// get back a list of ready fds.

#include <vector>

#ifdef __linux__
// ── Linux: epoll ──────────────────────────────────────────────────────────
#include <sys/epoll.h>
#include <unistd.h>

static constexpr int MAX_EVENTS = 64;

class Poller {
public:
    Poller() : fd_(epoll_create1(0)) {}
    ~Poller() { close(fd_); }

    void add(int fd) {
        epoll_event ev{};
        ev.events  = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(fd_, EPOLL_CTL_ADD, fd, &ev);
    }

    void remove(int fd) {
        epoll_ctl(fd_, EPOLL_CTL_DEL, fd, nullptr);
    }

    // Blocks until at least one fd is readable; fills ready with those fds.
    void wait(std::vector<int>& ready) {
        epoll_event events[MAX_EVENTS];
        int n = epoll_wait(fd_, events, MAX_EVENTS, -1);
        ready.clear();
        for (int i = 0; i < n; ++i)
            ready.push_back(events[i].data.fd);
    }

private:
    int fd_;
};

#else
// ── macOS / BSD: kqueue ───────────────────────────────────────────────────
#include <sys/event.h>
#include <unistd.h>

static constexpr int MAX_EVENTS = 64;

class Poller {
public:
    Poller() : fd_(kqueue()) {}
    ~Poller() { close(fd_); }

    void add(int fd) {
        struct kevent ev;
        EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
        kevent(fd_, &ev, 1, nullptr, 0, nullptr);
    }

    void remove(int fd) {
        struct kevent ev;
        EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
        kevent(fd_, &ev, 1, nullptr, 0, nullptr);
    }

    void wait(std::vector<int>& ready) {
        struct kevent events[MAX_EVENTS];
        int n = kevent(fd_, nullptr, 0, events, MAX_EVENTS, nullptr);
        ready.clear();
        for (int i = 0; i < n; ++i)
            ready.push_back(static_cast<int>(events[i].ident));
    }

private:
    int fd_;
};

#endif
