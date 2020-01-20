/**
  Author: sascha_lammers@gmx.de
*/

#pragma once

#include <Arduino_compat.h>

#ifndef DEBUG_EVENT_SCHEDULER
#define DEBUG_EVENT_SCHEDULER 0
#endif

class EventTimer;
class EventScheduler;

extern EventScheduler Scheduler;

class EventScheduler {
public:
    typedef enum {
        PRIO_NONE =         -1,
        PRIO_LOW =          0,
        PRIO_NORMAL,                // below HIGH, the timer callback is executed in the main loop and might be considerably delayed
        PRIO_HIGH,                  // HIGH runs the timer callback directly inside the timer interrupt/task without any delay
    } Priority_t;

    typedef enum {
        NO_CHANGE =         -2,
        UNLIMTIED =         -1,
        DONT =              0,
        ONCE =              1,
    } Repeat_t;

    typedef EventTimer *TimerPtr;
    typedef std::function<void(TimerPtr timer)> Callback;
    typedef std::vector<TimerPtr> TimerVector;
    typedef std::vector<TimerPtr>::iterator TimerIterator;

public:
    class Timer {
    public:
        typedef std::function<void(Timer &timer)> Callback_t;

        Timer() : _timer(nullptr) {
        }
        ~Timer() {
            remove();
        }

        void add(int64_t delayMillis, int repeat, Callback callback, Priority_t priority = PRIO_LOW) {
            remove();
            Scheduler.addTimer(&_timer, delayMillis, repeat, callback, priority);
        }

        void add(int64_t delayMillis, bool repeat, Callback callback, Priority_t priority = PRIO_LOW) {
            remove();
            Scheduler.addTimer(&_timer, delayMillis, repeat, callback, priority);
        }

        void add(int64_t delayMillis, int repeat, Callback_t callback, Priority_t priority = PRIO_LOW) {
            remove();
            Scheduler.addTimer(&_timer, delayMillis, repeat, [this, callback](EventScheduler::TimerPtr) {
                callback(*this);
            }, priority);
        }

        void add(int64_t delayMillis, bool repeat, Callback_t callback, Priority_t priority = PRIO_LOW) {
            add(delayMillis, (int)(repeat ? EventScheduler::UNLIMTIED : EventScheduler::DONT), callback, priority);
        }

        bool active() const {
            return Scheduler.hasTimer(_timer);
        }

        bool remove() {
            return Scheduler.removeTimer(&_timer);
        }

        EventScheduler::TimerPtr operator->() const {
            return _timer;
        }

    private:
        EventScheduler::TimerPtr _timer;
    };

public:
    EventScheduler() : _runtimeLimit(250) {
    }

    void begin();
    void end();

    // repeat as int = number of repeats
    void addTimer(TimerPtr *timerPtr, int64_t delayMillis, int repeat, Callback callback, Priority_t priority = PRIO_LOW);

    // repeat as bool = unlimited or none
    void addTimer(TimerPtr *timerPtr, int64_t delayMillis, bool repeat, Callback callback, Priority_t priority = PRIO_LOW) {
        addTimer(timerPtr, delayMillis, (int)(repeat ? EventScheduler::UNLIMTIED : EventScheduler::DONT), callback, priority);
    }

    void addTimer(int64_t delayMillis, int repeat, Callback callback, Priority_t priority = PRIO_LOW) {
        addTimer(nullptr, delayMillis, repeat, callback, priority);
    }
    void addTimer(int64_t delayMillis, bool repeat, Callback callback, Priority_t priority = PRIO_LOW) {
        addTimer(nullptr, delayMillis, repeat, callback, priority);
    }

    // check if the pointer is a timer that exists.
    // Note: Pointers are not unqiue and can be assigned to a different timer after the the memory has been released
    // To avoid this use void addTimer(TimerPtr *timerPtr,  ....) and if the timer gets removed, timerPtr will be set to null
    bool hasTimer(TimerPtr timer) const;

    // remove timer and return true if it has been removed
    bool removeTimer(TimerPtr timer);

    // remove timer and set to null, even if the timer did not exist
    bool removeTimer(TimerPtr *timer);

    static void loop();
    void callTimer(TimerPtr timer);

    static void _timerCallback(void *arg);

    size_t size() const {
        return _timers.size();
    }

private:
#if DEBUG_EVENT_SCHEDULER
    void _list();
#endif

    void _removeTimer(TimerPtr timer);
    void _loop();

private:
    TimerVector _timers;
    int _runtimeLimit; // if a callback takes longer than this amount of time, the next callback will be delayed until the next loop function call
};
