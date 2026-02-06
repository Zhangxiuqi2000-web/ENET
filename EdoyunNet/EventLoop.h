#include "EpollTaskScheduler.h"
#include <vector>

class EventLoop
{
public:
    //不允许拷贝、赋值
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator= (const EventLoop&) = delete;
    EventLoop(uint32_t num_threads = -1);
    ~EventLoop();

    std::shared_ptr<TaskScheduler> GetTaskScheduler();
    TimerId AddTimer(const TimerEvent& event, uint32_t msec);
    void RemoveTimer(TimerId timerId);
    void UpdateChannel(ChannelPtr channel);
    void RemoveChannel(ChannelPtr& channel);
    void Loop();
    void Quit();

private:
    uint32_t nums_threads_ = 1;
    uint32_t index_ = 1;
    std::vector<std::shared_ptr<TaskScheduler>> task_schedulers_;
    std::vector<std::shared_ptr<std::thread>> threads_;
};