#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include <atomic>
#include <chrono>

class BackgroundTimer
{
private:
    std::thread TimerThread;
    std::atomic_bool IsDestroy;
    std::atomic_int Code;
    std::condition_variable CondVar;
    std::chrono::microseconds SleepTime;
    std::mutex Mtx;
public:
    BackgroundTimer()
    {
        IsDestroy.store(false);
        Code.store(0);
        TimerThread = std::thread([this] ()
        {
            std::mutex condVarMtx;
            std::unique_lock<std::mutex> lk(condVarMtx);

            while (!IsDestroy.load())
            {
                CondVar.wait(lk);
                Code.store(1);
                Mtx.lock();
                Mtx.unlock();
                CondVar.wait_for(lk, SleepTime);
                Code.store(0);
            }

            Code.store(2);
        });
    }

    template<class T = std::chrono::seconds>
    void StartTimer(uint64_t timeToSleep)
    {
        Mtx.lock();
        while (Code.load() == 0)
            CondVar.notify_one();
        SleepTime = T(timeToSleep);
        Mtx.unlock();
    }

    bool IsSleep()
    {
        return Code.load() == 1;
    }

    ~BackgroundTimer()
    {
        IsDestroy.store(true);

        while (Code.load() != 2)
            CondVar.notify_one();
        
        TimerThread.join();
    }
};

int main()
{
    BackgroundTimer t;

    std::string str;
    while (true)
    {
        getline(std::cin, str);

        if (str == "f")
            break;
        
        if (str == "i")
            std::cout << t.IsSleep() << std::endl;

        if (str[0] == 's')
            t.StartTimer(str[1] - 'a');
    }
}