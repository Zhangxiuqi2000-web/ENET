#include <map>
#include <unordered_map>
#include <thread>
#include <cstdint>
#include <functional>
#include <chrono>
#include <memory>

typedef std::function<bool(void)> TimerEvent;
typedef uint32_t TimerId;

class Timer
{
public:
    Timer(const TimerEvent& event, uint32_t msec)
        :event_callback_(event),
        interval_(msec){}

    ~Timer(){}

    static void Sleep(uint32_t msec)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    }

private:
    void SetNextTimeout(uint64_t time_point)
    {
        next_timeout_ = time_point + interval_;
    }

    int64_t getNextTimeout()
    {
        return next_timeout_;
    }

private:
    friend class TimerQueue;
    uint32_t interval_ = 0;
    uint64_t next_timeout_ = 0;
    TimerEvent event_callback_ = []{return false;};
};

class TimerQueue
{
public:
    TimerQueue(){}
    ~TimerQueue(){}

public:
    TimerId AddTimer(const TimerEvent& event, uint32_t msec);
    void RemoveTimer(TimerId timerId);
    void HandleTimerEvent();

protected:
    int64_t GetTimeNow();
private:
    uint32_t last_timer_id = 0;
    std::unordered_map<TimerId, std::shared_ptr<Timer>> timers_;
    std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> events_;

};