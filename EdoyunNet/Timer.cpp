#include "Timer.h"

TimerId TimerQueue::AddTimer(const TimerEvent &event, uint32_t msec)
{
    int64_t time_point = GetTimeNow();
    TimerId timer_id = ++last_timer_id;

    auto timer = std::make_shared<Timer>(event, msec);
    timer->SetNextTimeout(time_point);
    timers_.emplace(timer_id, timer);
    events_.emplace(std::pair<int64_t, TimerId>(time_point + msec, timer_id), timer);
    return TimerId();
}

void TimerQueue::RemoveTimer(TimerId timerId)
{
    auto iter = timers_.find(timerId);
    if(iter != timers_.end())
    {
        int64_t timeout = iter->second->getNextTimeout();
        events_.erase(std::pair<int64_t, TimerId>(timeout, timerId));
        timers_.erase(timerId);
    }
}

void TimerQueue::HandleTimerEvent()
{
    if(!timers_.empty())
    {
        int64_t time_point = GetTimeNow();
        while(!timers_.empty() && events_.begin()->first.first <= time_point)
        {
            TimerId timerid = events_.begin()->first.second;
            if(timerid){
                bool flag = events_.begin()->second->event_callback_();
                if (flag) // 反复执行
                {
                    events_.begin()->second->SetNextTimeout(time_point);
                    auto timerptr = std::move(events_.begin()->second);
                    events_.erase(events_.begin());
                    events_.emplace(std::pair<uint64_t, TimerId>(timerptr->getNextTimeout(), timerid), timerptr);
                }
                else // 一次性
                {
                    events_.erase(events_.begin());
                    timers_.erase(timerid);
                }
            }
        }
    }
}

int64_t TimerQueue::GetTimeNow()
{
    auto time_point = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time_point.time_since_epoch()).count();
}
