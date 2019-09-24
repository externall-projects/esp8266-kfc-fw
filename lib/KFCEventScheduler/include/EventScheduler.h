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

    struct ActiveCallbackTimer {
        EventTimer *timer;
        bool removed;
    };

    typedef EventTimer *TimerPtr;
    typedef std::function<void(TimerPtr timer)> Callback;
    typedef std::vector<ActiveCallbackTimer> CallbackVector;
    typedef std::vector<TimerPtr> TimerVector;
    typedef std::vector<TimerPtr>::iterator TimerIterator;

    EventScheduler() {
        _loopFunctionInstalled = false;
        _runtimeLimit = 250;
    }

    // repeat as int = number of repeats
    TimerPtr addTimer(int64_t delayMillis, int repeat, Callback callback, Callback removeCallback, Priority_t priority = PRIO_LOW);

    inline TimerPtr addTimer(int64_t delayMillis, int repeat, Callback callback, Priority_t priority = PRIO_LOW) {
        return addTimer(delayMillis, repeat, callback, nullptr, priority);
    }

    // repeat as bool = unlimited or none
    TimerPtr addTimer(int64_t delayMillis, bool repeat, Callback callback, Priority_t priority = PRIO_LOW) {
        return addTimer(delayMillis, (int)(repeat ? EventScheduler::UNLIMTIED : EventScheduler::DONT), callback, priority);
    }
    TimerPtr addTimer(int64_t delayMillis, bool repeat, Callback callback, Callback removeCallback, Priority_t priority = PRIO_LOW) {
        return addTimer (delayMillis, (int)(repeat ? EventScheduler::UNLIMTIED : EventScheduler::DONT), callback, removeCallback, priority);
    }
    bool hasTimer(TimerPtr timer);
    void removeTimer(TimerPtr timer, bool deleteObject = true);

    void loopFunc();
    void callTimer(TimerPtr timer);
    void installLoopFunc();
    void removeLoopFunc();

    // this stops the scheduler and removes all timers, but does not invoke any callbacks and does not free memory
    void terminate();

    static void _timerCallback(void *arg);

    void _list();

private:
    CallbackVector _callbacks;
    TimerVector _timers;
    bool _loopFunctionInstalled;
    int _runtimeLimit; // if a callback takes longer than this amount of time, the next callback will be delayed until the next loop function call
};
