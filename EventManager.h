#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H
#include <map>
#include <queue>
#include <shared_mutex>

#include "Delegate.h"
#include "Object.h"

namespace Event {
    class ThreadPool {
    public:
        using Task = std::function<void()>;

        ThreadPool(size_t numThreads);

        ~ThreadPool();

        template<class F, class... Args>
        auto enqueue(F&&f, Args&&... args) -> std::future<typename std::invoke_result<F, Args...>::type>;

    private:
        std::vector<std::thread> workers;
        std::queue<Task> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    };

    class Signal : public std::enable_shared_from_this<Signal> {
    public:
        enum State {
            ON,
            OFF
        };

        static std::shared_ptr<Signal> Register(const std::string&sName);

        ~Signal();

        std::string toString() const;

        UUID ID() const;

        void SetState(State state);

        bool IsRecived() const;

        void Trigger(const std::vector<Object>&args);

        void UnTrigger();

        void AddListener(const Delegate<void (std::vector<Object>)>&func);

        void DeRegister();

        Signal* operator =(Signal&pSignal);

        std::shared_ptr<Signal> operator =(std::shared_ptr<Signal> pSignal);

        class Manager {
        public:
            static inline void Register(const std::shared_ptr<Signal>&pSignal);

            static inline std::shared_ptr<Signal> GetSignal(const UUID pID);

            static inline std::shared_ptr<Signal> GetSignal(const std::string&sName);

        private:
            static inline std::unordered_map<std::string, std::shared_ptr<Signal>> sSignalMap = {};
            static inline std::unordered_map<std::string, std::string> sSignalName = {};
        };

    private:
        void internalSetState(State state);

    protected:
        std::atomic<State> pRecived{OFF};
        std::string sName;
        UUID sID;
    };

    class EventManager {
    public:
        static inline void BroadcastWithThreadPool(const std::shared_ptr<Signal>&pSignal,
                                                   const std::vector<Object>&args = {});

        static inline void BroadcastAsync(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args = {});

        static inline void Broadcast(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args = {});

        static inline void UnBroadcast(const std::shared_ptr<Signal>&pSignal);

        static inline void AddListener(const std::shared_ptr<Signal>&pSignal,
                                       const auto&func);

        static inline void RemoveListener(const std::shared_ptr<Signal>&pSignal,
                                          const Delegate<void (std::vector<Object>)>&func);

        static inline bool Received(const std::shared_ptr<Signal>&pSignal);

        static void DeRegister(Signal&pSignal);

        static std::shared_ptr<Signal> Register(const std::string&sName);

        static inline void BoradcastWithThreadPool(const UUID pID, const std::vector<Object>&args = {});

        static inline void BroadcastAsync(const UUID pID, const std::vector<Object>&args = {});

        static inline void Broadcast(const UUID pID, const std::vector<Object>&args = {});

        static inline void UnBroadcast(const UUID pID);

        static inline void AddListener(const UUID pID, const Delegate<void (std::vector<Object>)>&func);

        static inline void RemoveListener(const UUID pID, const Delegate<void(std::vector<Object>)>&func);

        static inline bool Received(const UUID pID);

        static inline void DeRegister(const UUID pID);

    private:
        static inline std::unordered_map<std::string, std::tuple<std::vector<std::optional<Delegate<void (
                std::vector<Object>)>>>,
            std::vector<
                Object>>>
        sSignalMap;
        static inline std::shared_mutex sSignalMapLock;
    };
}
#endif //EVENTMANAGER_H
