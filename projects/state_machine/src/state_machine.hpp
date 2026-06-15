#pragma once

#include <functional>
#include <unordered_map>

// Generic finite state machine.
// State and Event must be enum class with underlying integer type.
template<typename State, typename Event>
class StateMachine {
public:
    using Action = std::function<void()>;

    explicit StateMachine(State initial) : current_(initial) {}

    // Register per-state lifecycle callbacks.
    void on_enter(State s, Action a) { enter_[state_key(s)] = std::move(a); }
    void on_exit (State s, Action a) { exit_ [state_key(s)] = std::move(a); }

    // Register a transition: (from, event) → to, with an optional mid-transition action.
    void add(State from, Event ev, State to, Action action = nullptr) {
        transitions_[tr_key(from, ev)] = {to, std::move(action)};
    }

    // Drive the machine with an event.
    // Returns true if a transition fired, false if the event is ignored.
    bool process(Event ev) {
        auto it = transitions_.find(tr_key(current_, ev));
        if (it == transitions_.end()) return false;

        auto& [key, tr] = *it;

        call(exit_,  state_key(current_));   // 1. exit current state
        if (tr.action) tr.action();           // 2. transition action
        current_ = tr.to;                     // 3. move to new state
        call(enter_, state_key(current_));    // 4. enter new state

        return true;
    }

    State current() const { return current_; }

private:
    struct Transition { State to; Action action; };

    // Encode (from, event) pair into a single key.
    // Assumes State and Event values fit in 16 bits each.
    static std::size_t tr_key(State from, Event ev) {
        return (static_cast<std::size_t>(from) << 16) |
                static_cast<std::size_t>(ev);
    }
    static std::size_t state_key(State s) {
        return static_cast<std::size_t>(s);
    }

    void call(std::unordered_map<std::size_t, Action>& table, std::size_t key) {
        auto it = table.find(key);
        if (it != table.end() && it->second) it->second();
    }

    State                                          current_;
    std::unordered_map<std::size_t, Transition>   transitions_;
    std::unordered_map<std::size_t, Action>        enter_;
    std::unordered_map<std::size_t, Action>        exit_;
};
