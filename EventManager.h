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
        enum class TriggerType : unsigned int {
            NORMAL = 1 << 0,
            ONCE,
            THREADPOOL,
            ASYNC,
            MULTIPLE,
        };

        // 重载位或运算符 |
        friend TriggerType operator|(TriggerType lhs, TriggerType rhs){
            return static_cast<TriggerType>(static_cast<unsigned int>(lhs) | static_cast<unsigned int>(rhs));
        }

        // 重载位与运算符 &
        friend TriggerType operator&(TriggerType lhs, TriggerType rhs){
            return static_cast<TriggerType>(static_cast<unsigned int>(lhs) & static_cast<unsigned int>(rhs));
        }

        // 重载左移运算符 <<
        friend std::ostream& operator<<(std::ostream&os, TriggerType type)  {
            switch (type) {
                case TriggerType::NORMAL:
                    os << "NORMAL";
                    break;
                case TriggerType::ONCE:
                    os << "ONCE";
                    break;
                case TriggerType::THREADPOOL:
                    os << "THREADPOOL";
                    break;
                case TriggerType::ASYNC:
                    os << "ASYNC";
                    break;
                case TriggerType::MULTIPLE:
                    os << "MULTIPLE";
                    break;
                default:
                    os << "Unknown";
                    break;
            }
            return os;
        }

        // 重载位取反运算符 ~
        friend TriggerType operator~(TriggerType value) {
            return static_cast<TriggerType>(~static_cast<unsigned int>(value));
        }

        // 重载异或运算符 ^
        friend TriggerType operator^(TriggerType lhs, TriggerType rhs) {
            return static_cast<TriggerType>(static_cast<unsigned int>(lhs) ^ static_cast<unsigned int>(rhs));
        }

        // 重载比较运算符 ==
        friend bool operator==(TriggerType lhs, unsigned int rhs) {
            return static_cast<unsigned int>(lhs) == rhs;
        }

        // 重载比较运算符 !=
        friend bool operator!=(TriggerType lhs, unsigned int rhs) {
            return static_cast<unsigned int>(lhs) != rhs;
        }

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

        void Trigger(const std::vector<Object>&args = {}, TriggerType type = TriggerType::ONCE | TriggerType::NORMAL);

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
        static void BroadcastWithThreadPool(const std::shared_ptr<Signal>&pSignal,
                                            const std::vector<Object>&args = {});

        static void BroadcastAsync(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args = {});

        static void Broadcast(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args = {});

        static void BroadcastMultiple(const std::shared_ptr<Signal>&pSignal, const std::vector<Object>&args = {});

        static void UnBroadcast(const std::shared_ptr<Signal>&pSignal);

        static void AddListener(const std::shared_ptr<Signal>&pSignal,
                                const auto&func);

        static void RemoveListener(const std::shared_ptr<Signal>&pSignal,
                                   const Delegate<void (std::vector<Object>)>&func);

        static bool Received(const std::shared_ptr<Signal>&pSignal);

        static void DeRegister(Signal&pSignal);

        static std::shared_ptr<Signal> Register(const std::string&sName);

        static void BoradcastWithThreadPool(const UUID pID, const std::vector<Object>&args = {});

        static void BroadcastAsync(const UUID pID, const std::vector<Object>&args = {});

        static void Broadcast(const UUID pID, const std::vector<Object>&args = {});

        static void BroadcastMultiple(const UUID pID, const std::vector<Object>&args = {});

        static void UnBroadcast(const UUID pID);

        static void AddListener(const UUID pID, const Delegate<void (std::vector<Object>)>&func);

        static void RemoveListener(const UUID pID, const Delegate<void(std::vector<Object>)>&func);

        static bool Received(const UUID pID);

        static void DeRegister(const UUID pID);

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
