#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H
#include <map>
#include <queue>
#include <shared_mutex>

#include "Delegate.h"
#include "Object.h"

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back(
                [this] {
                    for (;;) {
                        std::function<void()> task; {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                                 [this] { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        task();
                    }
                }
            );
        }
    }

    ~ThreadPool() { {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread&worker: workers) {
            worker.join();
        }
    }

    template<class F, class... Args>
    auto enqueue(F&&f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future(); {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (tasks.empty()) {
                condition.notify_one();
            }
            tasks.push([task]() { (*task)(); });
        }
        return res;
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

class Signal {
public:
    enum State {
        ON,
        OFF
    };

    static std::shared_ptr<Signal> Create(const std::string&sName) {
        std::shared_ptr<Signal> _si = std::make_shared<Signal>();
        _si->sName = sName;
        _si->sID = UUID::New();
        return _si;
    }

    ~Signal() {
        pRecived.store(OFF);
    }

    std::string toString() const {
        return sName;
    }

    UUID ID() const {
        return sID;
    }

    void SetState(State state) {
        std::async(std::launch::async, &Signal::internalSetState, this, state).wait();
    }

    bool IsRecived() const {
        return pRecived.load() == ON;
    }

    void Emit(const std::vector<Object>&args);

    void AddListener(const Delegate<void (std::vector<Object>)>&func);

    void DeRegister();

    Signal* operator =(Signal&pSignal) {
        sName = pSignal.sName;
        sID = pSignal.sID;
        return this;
    }

    std::shared_ptr<Signal> operator =(std::shared_ptr<Signal> pSignal) {
        sName = pSignal->sName;
        sID = pSignal->sID;
        return std::shared_ptr<Signal>(this, [](Signal*) {
        });;
    }

    class Manager {
    public:
        static inline void Register(std::shared_ptr<Signal> pSignal) {
            sSignalMap[pSignal->ID().toString()] = pSignal;
        }

        static inline std::shared_ptr<Signal> GetSignal(UUID pID) {
            return sSignalMap[pID.toString()];
        }

    private:
        static inline std::map<std::string, std::shared_ptr<Signal>> sSignalMap = {};
    };

private:
    void internalSetState(State state) {
        pRecived.store(state);
    }

protected:
    std::atomic<State> pRecived{OFF};
    std::string sName;
    UUID sID;
};

class EventManager {
public:
    static inline void BroadcastWithThreadPool(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);
        pSignal->SetState(Signal::ON);
        auto pID = pSignal->ID();
        if (!sSignalMap.contains(pID.toString())) {
            sSignalMap[pID.toString()] = std::make_tuple(
                std::vector<std::optional<Delegate<void (std::vector<Object>)>>>(),
                std::vector<Object>());
        }
        else {
            auto&[funcList, argsList] = sSignalMap[pID.toString()];
            argsList = args;
            ThreadPool pool(std::thread::hardware_concurrency()); // 创建一个线程池，大小为硬件并发数

            // 使用线程池执行每个注册的处理函数
            std::vector<std::future<void>> futures;
            for (auto&func: funcList) {
                if (func.has_value()) {
                    futures.emplace_back(pool.enqueue([func, args] {
                        func.value()(args);
                    }));
                }
            }

            // 等待所有异步任务完成
            for (auto&future: futures) {
                future.wait();
            }
            futures.clear();
        }
    }

    static inline void BroadcastAsync(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);
        pSignal->SetState(Signal::ON);
        auto pID = pSignal->ID();
        if (!sSignalMap.contains(pID.toString())) {
            sSignalMap[pID.toString()] = std::make_tuple(
                std::vector<std::optional<Delegate<void (std::vector<Object>)>>>(),
                std::vector<Object>());
        }
        else {
            auto&[funcList, argsList] = sSignalMap[pID.toString()];
            argsList = args;

            // 存储所有的 std::future
            std::vector<std::future<void>> futures;

            // 异步执行每个注册的处理函数
            for (auto&func: funcList) {
                if (func.has_value())
                    futures.emplace_back(std::async(std::launch::async, func.value(), args));
            }

            // 等待所有异步任务完成
            for (auto&future: futures) {
                future.wait();
            }
            futures.clear();
        }
    }

    static inline void Broadcast(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);
        pSignal->SetState(Signal::ON);
        auto pID = pSignal->ID();
        if (!sSignalMap.contains(pID.toString())) {
            sSignalMap[pID.toString()] = std::make_tuple(
                std::vector<std::optional<Delegate<void (std::vector<Object>)>>>(),
                std::vector<Object>());
        }
        else {
            auto&[funcList, argsList] = sSignalMap[pID.toString()];
            argsList = args;

            for (auto&func: funcList) {
                if (func.has_value())
                    func.value()(args);
            }
        }
    }

    static inline void UnBroadcast(const std::shared_ptr<Signal>&pSignal) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);
        pSignal->SetState(Signal::OFF);
    }

    static inline void AddListener(const std::shared_ptr<Signal>&pSignal,
                                   const Delegate<void (std::vector<Object>)>&func) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);

        if (!sSignalMap.contains(pSignal->ID().toString())) {
            sSignalMap[pSignal->ID().toString()] = std::make_tuple(
                std::vector<std::optional<Delegate<void (std::vector<Object>)>>>{func},
                std::vector<Object>());
        }
        else {
            auto&[funcList, argsList] = sSignalMap[pSignal->ID().toString()];
            funcList.emplace_back(func);
        }
    }

    static inline void RemoveListener(const std::shared_ptr<Signal>&pSignal,
                                      const Delegate<void (std::vector<Object>)>&func) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);

        if (const auto it = sSignalMap.find(pSignal->ID().toString()); it != sSignalMap.end()) {
            auto&[funcList, argsList] = it->second;
            std::erase_if(funcList,
                          [&](const std::optional<Delegate<void (std::vector<Object>)>>&optFunc) {
                              return optFunc.has_value() && (optFunc.value() == func);
                          });
        }
    }

    static inline bool Received(const std::shared_ptr<Signal>&pSignal) {
        std::shared_lock<std::shared_mutex> lock(sSignalMapLock);
        return pSignal->IsRecived();
    }

    static void DeRegister(Signal&pSignal) {
        std::unique_lock<std::shared_mutex> lock(sSignalMapLock);
        sSignalMap.erase(pSignal.ID().toString());
        pSignal.~Signal();
    }

    static inline void Boradcast(const UUID pID, const std::vector<Object>&args) {
        BroadcastWithThreadPool(Signal::Manager::GetSignal(pID), args);
    }

    static inline void UnBroadcast(const UUID pID) {
        UnBroadcast(Signal::Manager::GetSignal(pID));
    }

    static inline void AddListener(const UUID pID, const Delegate<void (std::vector<Object>)>&func) {
        AddListener(Signal::Manager::GetSignal(pID), func);
    }

    static inline void RemoveListener(const UUID pID, const Delegate<void(std::vector<Object>)>&func) {
        RemoveListener(Signal::Manager::GetSignal(pID), func);
    }

    static inline bool Received(const UUID pID) {
        return Received(Signal::Manager::GetSignal(pID));
    }

    static inline void DeRegister(const UUID pID) {
        DeRegister(*Signal::Manager::GetSignal(pID));
    }

private:
    static inline std::unordered_map<std::string, std::tuple<std::vector<std::optional<Delegate<void (
            std::vector<Object>)>>>,
        std::vector<
            Object>>>
    sSignalMap;
    static inline std::shared_mutex sSignalMapLock;
};

inline void Signal::Emit(const std::vector<Object>&args) {
    EventManager::BroadcastWithThreadPool(std::shared_ptr<Signal>(this, [](Signal*) {
    }), args);
}

inline void Signal::AddListener(const Delegate<void(std::vector<Object>)>&func) {
    EventManager::AddListener(std::shared_ptr<Signal>(this, [](Signal*) {
    }), func);
}

inline void Signal::DeRegister() {
    EventManager::DeRegister(*this);
}

#endif //EVENTMANAGER_H
