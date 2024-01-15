#include <utility>

#include "Delegate.h"


class StopWatch {
public:
    enum TimeType {
        Microseconds,
        Milliseconds,
        Seconds,
        Minutes,
        Hours,
    };

    void Start() {
        m_start = std::chrono::steady_clock::now();
    }

    double Duration() const {
        return static_cast<double>(m_duration);
    }

    void End() {
        m_end = std::chrono::steady_clock::now();
        m_duration = std::chrono::duration_cast<std::chrono::microseconds>(m_end - m_start).count();
    }

    template<typename _T, typename... Args>
    std::chrono::microseconds::rep TestFunc(Delegate<_T> func, Args... args) {
        Start();
        func(std::forward<Args>(args)...);
        End();
        return Duration();
    }

    template<typename _T>
    std::chrono::microseconds::rep TestFunc(std::function<_T()> func) {
        Start();
        func();
        End();
        return Duration();
    }

    template<typename _T, typename... Args>
    std::chrono::microseconds::rep
    TestFunctionMultithread(Delegate<_T> func, int numIterations = 1000, Args... args) {
        Start();

        std::vector<std::future<void>> futures;
        futures.reserve(numIterations);

        for (int i = 0; i < numIterations; ++i) {
            futures.emplace_back(std::async(std::launch::async, [&func, &args...]() {
                func(std::forward<Args>(args)...);
            }));
        }

        // 等待所有异步任务完成
        for (auto&future: futures) {
            future.wait_for(std::chrono::seconds(10));
        }

        End();

        // 返回平均时间
        return Duration() / numIterations;
    }

    template<typename _T>
    std::chrono::microseconds::rep TestFunctionMultithread(std::function<_T()> func, int numIterations = 1000) {
        Start();

        std::vector<std::future<void>> futures;
        futures.reserve(numIterations);

        for (int i = 0; i < numIterations; ++i) {
            futures.emplace_back(std::async(std::launch::async, func));
        }

        // 等待所有异步任务完成
        for (auto&future: futures) {
            future.wait_for(std::chrono::seconds(10));
        }

        End();

        // 返回平均时间
        return Duration() / numIterations;
    }

private:
    std::chrono::steady_clock::time_point m_start;
    std::chrono::steady_clock::time_point m_end;
    std::chrono::microseconds::rep m_duration{};
};

class Object {
public:
    template<typename _T>
    Object(_T data): pData(data) {
    }

    template<typename _T>
    _T Cast() const {
        if (!pData.has_value()) {
            throw std::runtime_error("Data is empty!");
        }
        if (typeid(_T) != pData.value().type()) {
            throw std::runtime_error("Type mismatch!");
        } {
            try {
                return std::any_cast<_T>(pData.value());
            }
            catch (const std::bad_any_cast&e) {
                throw std::runtime_error("Cannot cast data to " + std::string(typeid(_T).name()));
            }
        }
    }

private:
    std::optional<std::any> pData;
};

class Foo {
public:
    Foo() = default;

    void func(const std::vector<Object>&args) {
        for (int i = 0; i < 1000; i++) {
            a = i * i;
        }
    }

    int a = 0;
};

class Signal {
public:
    enum State {
        ON,
        OFF
    };

    static std::shared_ptr<Signal> Create(std::string sName) {
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
        std::async(std::launch::async, &Signal::internalSetState, this, state);
    }

    bool IsRecived() const {
        return pRecived.load() == ON;
    }

    void Emit(const std::vector<Object>&args);

    void AddListener(Delegate<void (std::vector<Object>)> func);

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
    static inline void Broadcast(std::shared_ptr<Signal> pSignal, const std::vector<Object>&args) {
        std::lock_guard<std::mutex> lock(sSignalMapLock);
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
                future.wait_for(std::chrono::seconds(10));
            }
        }
    }

    static inline void UnBroadcast(std::shared_ptr<Signal> pSignal) {
        std::lock_guard<std::mutex> lock(sSignalMapLock);
        pSignal->SetState(Signal::OFF);
    }

    static inline void AddListener(std::shared_ptr<Signal> pSignal, const Delegate<void (std::vector<Object>)>&func) {
        std::lock_guard<std::mutex> lock(sSignalMapLock);

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

    static inline void RemoveListener(std::shared_ptr<Signal> pSignal,
                                      const Delegate<void (std::vector<Object>)>&func) {
        std::lock_guard<std::mutex> lock(sSignalMapLock);

        if (!sSignalMap.contains(pSignal->ID().toString())) {
            return;
        }

        auto&[funcList, argsList] = sSignalMap[pSignal->ID().toString()];

        funcList.erase(std::remove_if(funcList.begin(), funcList.end(),
                                      [&](const std::optional<Delegate<void (std::vector<Object>)>>&optFunc) {
                                          return optFunc.has_value() && (optFunc.value() == func);
                                      }), funcList.end());
    }

    static inline bool Received(std::shared_ptr<Signal> pSignal) {
        std::lock_guard<std::mutex> lock(sSignalMapLock);
        return pSignal->IsRecived();
    }

    static inline void DeRegister(Signal&pSignal) {
        if (sSignalMap.contains(pSignal.ID().toString())) {
            sSignalMap.erase(pSignal.ID().toString());
            auto si = std::shared_ptr<Signal>(&pSignal, [](Signal*) {
            });
            Signal::Manager::Register(si);
            pSignal.~Signal();
        }
    }

    static inline void Boradcast(UUID pID, const std::vector<Object>&args) {
        Broadcast(Signal::Manager::GetSignal(pID), args);
    }

    static inline void UnBroadcast(UUID pID) {
        UnBroadcast(Signal::Manager::GetSignal(pID));
    }

    static inline void AddListener(UUID pID, const Delegate<void (std::vector<Object>)>&func) {
        AddListener(Signal::Manager::GetSignal(pID), func);
    }

    static inline void RemoveListener(UUID pID, const Delegate<void (std::vector<Object>)>&func) {
        RemoveListener(Signal::Manager::GetSignal(pID), func);
    }

    static inline bool Received(UUID pID) {
        return Received(Signal::Manager::GetSignal(pID));
    }

    static inline void DeRegister(UUID pID) {
        DeRegister(*Signal::Manager::GetSignal(pID));
    }

private:
    static inline std::map<std::string, std::tuple<std::vector<std::optional<Delegate<void (std::vector<Object>)>>>,
        std::vector<
            Object>>>
    sSignalMap;
    static inline std::mutex sSignalMapLock;
};

void Signal::Emit(const std::vector<Object>&args) {
    EventManager::Broadcast(std::shared_ptr<Signal>(this, [](Signal*) {
    }), args);
}

void Signal::AddListener(Delegate<void(std::vector<Object>)> func) {
    EventManager::AddListener(std::shared_ptr<Signal>(this, [](Signal*) {
    }), func);
}

void Signal::DeRegister() {
    EventManager::DeRegister(*this);
}


// 示例函数
int add(int a, int b) {
    return a + b;
}

void func(const std::vector<Object>&args) {
}

// 示例类
class Calculator {
public:
    int multiply(int a, int b) {
        return a * b;
    }

    int foo(int a, int b, int c) {
        return a * b;
    }

    void func(const std::vector<Object>&args) {
        for (int i = 0; i < 1000; i++) {
            int a = i * i;
        }
    }
};

std::string TimeFormat(std::chrono::microseconds::rep duration) {
    using namespace std::chrono;

    auto ms = duration_cast<milliseconds>(microseconds(duration)).count();
    auto s = duration_cast<seconds>(microseconds(duration)).count();
    auto min = duration_cast<minutes>(microseconds(duration)).count();
    auto hr = duration_cast<hours>(microseconds(duration)).count();

    std::string result;
    if (hr > 0) {
        result += std::to_string(hr) + "hr ";
    }
    if (min > 0) {
        result += std::to_string(min % 60) + "min ";
    }
    if (s > 0) {
        result += std::to_string(s % 60) + "s ";
    }
    if (ms > 0) {
        result += std::to_string(ms % 1000) + "ms ";
    }
    else {
        result += std::to_string(duration) + "us";
    }

    return result.empty() ? "0us" : result;
}

int main() {
    try {
        StopWatch stopwatch;

        auto si = Signal::Create("test");
        Foo foo;

        Calculator calculator;
        Delegate<void(std::vector<Object>)> t1(func);
        Delegate<int (Calculator::*)(int, int)> t2(Calculator::multiply, calculator);

        Delegate<int (Calculator::*)(int, int, int)> t3(Calculator::foo, calculator);
        Delegate<int (int, int)> f2 = t2;
        stopwatch.TestFunctionMultithread(f2, 1000, 6, 6);
        Delegate<void(Calculator::*)(std::vector<Object>)> t4(Calculator::func, calculator);
        EventManager::AddListener(si, t4);
        Delegate<void()> t6([&] {
            EventManager::Broadcast(si, {1, 2, 3});
        });
        /*std::jthread([&] {
            std::cout << TimeFormat(stopwatch.TestFunctionMultithread(t6, 1000)) << std::endl;
        });*/
        std::jthread([&] {
            std::cout << TimeFormat(stopwatch.TestFunctionMultithread(t4, 1000, std::vector<Object>{1, 2, 3})) <<
                    std::endl;
        });
    }
    catch (const std::exception&e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
