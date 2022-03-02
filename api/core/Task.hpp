#pragma once

class Task {
  public:
    Task() {}

    template <typename Func, typename... Args>
    void start(Func function, Args... args) {
        running = true;
        thread = std::thread(function, args...);
    }

    void stop(long long int timeout){
    }

    virtual bool join(){
        if(thread.joinable()){
            thread.join();
            running = false;
            return true;
        }
        return false;
    }


    bool isRunning(){ return running; }

  protected:
    bool running = false;
    std::thread thread;
};
