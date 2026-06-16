#pragma once

#include <algorithm>
#include <memory>
#include <vector>

// Base observer interface.
// Subclass and override on_event() to react to notifications.
template<typename Event>
class Observer {
public:
    virtual void on_event(const Event& e) = 0;
    virtual ~Observer() = default;
};

// Subject (Observable): maintains a list of observers and notifies them.
// Observers are held as non-owning raw pointers — the caller owns the lifetime.
template<typename Event>
class Subject {
public:
    void subscribe(Observer<Event>* obs) {
        observers_.push_back(obs);
    }

    void unsubscribe(Observer<Event>* obs) {
        observers_.erase(
            std::remove(observers_.begin(), observers_.end(), obs),
            observers_.end());
    }

protected:
    void notify(const Event& e) {
        for (auto* obs : observers_) obs->on_event(e);
    }

private:
    std::vector<Observer<Event>*> observers_;
};
