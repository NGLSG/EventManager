#ifndef DELEGATE_H
#define DELEGATE_H
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <any>
#include <vector>
#include <functional>
#include <map>

class UUID {
public:
    UUID() = default;

    static UUID New() {
        UUID uuid;
        static std::random_device rd;
        static std::mt19937 generator(rd());
        static std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);

        // 生成四个32位的随机数
        for (unsigned int&i: uuid.data) {
            i = distribution(generator);
        }
        return uuid;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (int i = 0; i < 4; ++i) {
            ss << std::setw(8) << data[i];
            if (i < 3) {
                ss << '-';
            }
        }
        return ss.str();
    }

    bool operator==(const UUID&rhs) const {
        return toString() == rhs.toString();
    }

private:
    uint32_t data[4]{};
};

// Delegate class
template<typename T>
class Delegate;

// Specialization for member functions
template<typename R, typename C, typename... Args>
class Delegate<R (C::*)(Args...)> {
public:
    using FunctionType = std::function<R(C&, Args...)>;

    Delegate(FunctionType func, C&instance) : pDelegate(func), pInstance(&instance) {
        pID = UUID::New();
    }

    bool operator==(const Delegate&rhs) const {
        return pID == rhs.pID;
    }

    R operator()(Args... args) const {
        if (pInstance) {
            return pDelegate(*pInstance, args...);
        }
        throw std::runtime_error("Instance is null");
    }

    operator Delegate<R(Args...)>() const {
        auto lambda = [&](Args... args) {
            return pDelegate(*pInstance, args...);
        };

        return Delegate<R(Args...)>(lambda);
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
    friend class Delegate;
    using FunctionType = std::function<R(Args...)>;

    Delegate(FunctionType func) : pDelegate(func) {
        pID = UUID::New();
    }

    bool operator==(const Delegate&rhs) const {
        return pID == rhs.pID;
    }

    R operator()(Args... args) const {
        return pDelegate(args...);
    }

private:
    FunctionType pDelegate;
    UUID pID;
};


#endif //DELEGATE_H
