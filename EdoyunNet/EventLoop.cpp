#include "EventLoop.h"

EventLoop::EventLoop(uint32_t num_threads)
    :index_(1),
    nums_threads_(num_threads)
{
    this->Loop();
}

EventLoop::~EventLoop()
{
    this->Quit();
}

std::shared_ptr<TaskScheduler> EventLoop::GetTaskScheduler()
{
    if(task_schedulers_.size() == 1)
    {
        return task_schedulers_.at(0);
    }
    else
    {
        auto task_scheduler = task_schedulers_.at(index_);
        index_++;
        if(index_ >= task_schedulers_.size())
        {
            index_ = 0;
        }
        return task_scheduler;
    }
    return nullptr;
}

TimerId EventLoop::AddTimer(const TimerEvent &event, uint32_t msec)
{
    if(task_schedulers_.size() > 0)
    {
        return task_schedulers_[0]->AddTimer(event, msec);  //0是主反应器
    }
    return 0;
}

void EventLoop::RemoveTimer(TimerId timerId)
{
    if(task_schedulers_.size() > 0)
    {
        return task_schedulers_[0]->RemoveTimer(timerId);
    }
}

void EventLoop::UpdateChannel(ChannelPtr channel)
{
    if(task_schedulers_.size() > 0)
    {
        task_schedulers_[0]->UpdateChannel(channel);
    }
}

void EventLoop::RemoveChannel(ChannelPtr &channel)
{
    if(task_schedulers_.size() > 0)
    {
        task_schedulers_[0]->RemoveChannel(channel);
    }
}

void EventLoop::Loop()
{
    if(task_schedulers_.empty())
    {
        for(uint32_t n = 0; n < nums_threads_; n++)
        {
            std::shared_ptr<TaskScheduler> task_scheduler_ptr(new EpollTaskScheduler);
            task_schedulers_.push_back(task_scheduler_ptr);
            //创建一个新线程，在这个线程中调用task_scheduler_ptr所指向的对象的Start()成员函数
            std::shared_ptr<std::thread> thread(new std::thread(&TaskScheduler::Start, task_scheduler_ptr.get()));
            //获取底层线程句柄
            thread->native_handle();
            threads_.push_back(thread);
        }
    }
}

void EventLoop::Quit()
{
    for(auto iter : task_schedulers_)
    {
        iter->Stop();
    }
    for(auto iter : threads_)
    {
        if(iter->joinable())
        {
            iter->join();
        }
    }
    task_schedulers_.clear();
    threads_.clear();
}
