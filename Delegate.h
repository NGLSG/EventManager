#ifndef DELEGATE_H
#define DELEGATE_H
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <functional>
#include <array>
#include <set>

namespace Event {
    struct UUID {
    public:
        UUID() : data({0}) {
        }

        static UUID New();

        std::string toString() const;

        std::array<uint32_t, 4> GetData() const {
            return data;
        }
        bool operator==(const UUID& other) const;

        bool operator<(const UUID& other) const;

        std::ostream& operator<<(std::ostream& os) const;

    private:
        inline static std::set<UUID> uuids;
        std::array<uint32_t, 4> data;
    };

    // Delegate class
    template<typename T>
    class Delegate;

    template<typename C, typename R, typename... Args>
    Delegate<R (C::*)(Args...)> MakeDelegate(R (C::*memberFunc)(Args...), std::shared_ptr<C> instance) {
        return Delegate<R (C::*)(Args...)>(memberFunc, instance);
    }

    template<typename C, typename R, typename... Args>
    Delegate<R (C::*)(Args...)> MakeDelegate(R (C::*memberFunc)(Args...), C&instance) {
        return Delegate<R (C::*)(Args...)>(memberFunc, std::make_shared<C>(instance));
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
        using InstancePtr = std::shared_ptr<C>;

        Delegate(FunctionType func, const InstancePtr&instance) : pDelegate(func), pInstance(instance) {
            pID = UUID::New();
        }

        // 转换构造函数，允许从成员函数对象初始化
        Delegate(R (C::*funcPtr)(Args...), const InstancePtr&instance) {
            pDelegate = [funcPtr](C&instance, Args... args) -> R {
                return (instance.*funcPtr)(std::forward<Args>(args)...);
            };
            pID = UUID::New();
            pInstance = instance; // 当使用成员函数对象初始化时，实例指针应该被传入
        }

        //不传入Instance进行构造
        Delegate(R (C::*func)(Args...), C* instance) : pDelegate([instance,func](Args... args) {
            return (instance->*func)(std::forward<Args>(args)...);
        }), pInstance(instance) {
            pID = UUID::New();
        }

        Delegate(): pDelegate(nullptr), pInstance(nullptr) {
            pID = UUID::New();
        }

        void SetInstance(const InstancePtr&instance) {
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

        R operator()(std::shared_ptr<C> instance, Args... args) const {
            return pDelegate(*instance, std::forward<Args>(args)...);
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

        UUID ID() const {
            return pID;
        }

    private:
        FunctionType pDelegate;
        UUID pID;
        InstancePtr pInstance; // Pointer to the instance
    };

    // Specialization for free functions
    template<typename R, typename... Args>
    class Delegate<R(Args...)> {
    public:
        using FunctionType = std::function<R(Args...)>;

        Delegate(FunctionType func) : pDelegate(func) {
            pID = UUID::New();
        }

        Delegate() {
            pID = UUID::New();
        }

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

        UUID ID() const {
            return pID;
        }

    private:
        FunctionType pDelegate;
        UUID pID;
    };

    inline static void DoNothing() {
    }
}

#endif //DELEGATE_H
