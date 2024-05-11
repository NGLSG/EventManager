#ifndef DELEGATE_H
#define DELEGATE_H
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <functional>
#include <array>

namespace Event {
    class UUID {
    public:
        UUID() : data({0}) {
        }

        static UUID New();

        std::string toString() const;

        friend bool operator==(const UUID&lhs, const UUID&rhs);

    private:
        std::array<uint32_t, 4> data;
    };

    // Delegate class
    template<typename T>
    class Delegate;

    template<typename C, typename R, typename... Args>
    Delegate<R (C::*)(Args...)> MakeDelegate(R (C::*memberFunc)(Args...), C&instance) {
        return Delegate<R (C::*)(Args...)>(memberFunc, instance);
    }

    template<typename C, typename R, typename... Args>
    Delegate<R (C::*)(Args...)> MakeDelegate(R (C::*memberFunc)(Args...)) {
        return Delegate<R (C::*)(Args...)>(memberFunc);
    }

    template<typename R, typename... Args>
    Delegate<R(Args...)> MakeDelegate(R (*func)(Args...)) {
        return Delegate<R(Args...)>(func);
    }

    // Specialization for member functions
    template<typename R, typename C, typename... Args>
    class Delegate<R (C::*)(Args...)> {
    public:
        using FunctionType = std::function<R(C&, Args...)>;

        Delegate(FunctionType func, C&instance) : pDelegate(func), pInstance(&instance) {
            pID = UUID::New();
        }

        // 转换构造函数，允许从成员函数对象初始化
        Delegate(R (C::*funcPtr)(Args...)) {
            pDelegate = [funcPtr](C&instance, Args... args) -> R {
                return (instance.*funcPtr)(std::forward<Args>(args)...);
            };
            pID = UUID::New();
            pInstance = nullptr; // 当使用成员函数对象初始化时，实例指针应保持为 nullptr
        }

        //不传入Instance进行构造
        Delegate(R (C::*func)(Args...), C* instance) : pDelegate([instance,func](Args... args) {
            return (instance->*func)(std::forward<Args>(args)...);
        }), pInstance(instance) {
            pID = UUID::New();
        }

        Delegate() = default;

        void SetInstance(C* instance) {
            pInstance = instance;
        }

        bool operator==(const Delegate&rhs) const {
            return pID == rhs.pID;
        }

        R operator()(Args... args) const {
            if (pInstance) {
                return pDelegate(*pInstance, std::forward<Args>(args)...);
            }
            throw std::runtime_error("Instance is null");
        }

        R operator()(C&instance, Args... args) const {
            return pDelegate(instance, std::forward<Args>(args)...);
        }

        //转为普通委托
        operator Delegate<R(Args...)>() const {
            auto lambda = [&](Args... args) {
                return pDelegate(*pInstance, std::forward<Args>(args)...);
            };

            return Delegate<R(Args...)>(lambda);
        }

        Delegate& operator=(const Delegate&rhs) {
            pDelegate = rhs.pDelegate;
            pInstance = rhs.pInstance;
            pID = rhs.pID;
            return *this;
        }

    private:
        FunctionType pDelegate;
        UUID pID;
        C* pInstance; // Pointer to the instance
    };

    // Specialization for free functions
    template<typename R, typename... Args>
    class Delegate<R(Args...)> {
    public:
        using FunctionType = std::function<R(Args...)>;

        explicit Delegate(FunctionType func) : pDelegate(func) {
            pID = UUID::New();
        }

        Delegate() = default;

        bool operator==(const Delegate&rhs) const {
            return pID == rhs.pID;
        }

        R operator()(Args... args) const {
            return pDelegate(std::forward<Args>(args)...);
        }

        Delegate& operator=(const Delegate&rhs) {
            pDelegate = rhs.pDelegate;
            pID = rhs.pID;
            return *this;
        }

        Delegate& operator=(FunctionType func) {
            pDelegate = func;
            return *this;
        }

        Delegate& operator=(R (*func)(Args...)) {
            pDelegate = [func](Args... args) {
                return func(args...);
            };
        }

    private:
        FunctionType pDelegate;
        UUID pID;
    };

    inline static void DoNothing() {
    }
}
#endif //DELEGATE_H
