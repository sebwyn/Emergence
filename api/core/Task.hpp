#pragma once

#include "Logger.hpp"

class Task {
  public:
    void start() {
        running.store(true);
        thread = std::thread([&]() {
            execute();
            running.store(false);
        });
    }

    bool tryJoin() {
        if (!running.load()) {
            Logger::logInfo("Joined");
            thread.join();
            running.store(false);
            onEnd();
            return true;
        }
        return false;
    }

    void join(){
        if(thread.joinable()){
            thread.join();
        }
    }

    bool getRunning() { return running.load(); }

  protected:
    virtual void execute() = 0;
    virtual void onEnd() {}

    std::thread thread;
    std::atomic<bool> running;
};

template <class T> class Threaded {
  public:
    Threaded() = default;
    Threaded(T _t) : t(_t) {}

    // Threaded(Threaded<T> &&other) = delete;
    // void operator==(T &&other) = delete;

    template <class F> auto access(F &&f) {
        auto lk = lock();
        f(std::ref(t));
    }

    template <class F> auto debugAccess(F &&f) {
        auto startedWaiting = std::chrono::high_resolution_clock::now();
        auto lk = lock();
        auto finishedWaiting = std::chrono::high_resolution_clock::now();
        std::cout << "Waited on mutex for: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(
                         finishedWaiting - startedWaiting).count()
                  << " microseconds" << std::endl;
        f(std::ref(t));
    }

    // not safe (only use outside threads)
    T &getValue() { return t; }
    void setValue(T &&value) { t = std::move(value); }

  private:
    T t;
    std::mutex m;

    auto lock() { return std::unique_lock<std::mutex>(m); }
};
