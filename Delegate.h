#ifndef DELEGATE_H
#define DELEGATE_H
#include <iostream>
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <functional>

class UUID {
public:
    UUID() = default;

    static UUID New() {
        UUID uuid;
        static std::random_device rd;
        static std::mt19937 generator(rd());
        static std::uniform_int_distribution<uint32_t> distribution(0, UINT32_MAX);

        // 生成四个32位的随机数
        for (auto&i: uuid.data) {
            i = distribution(generator);
        }

        // 设置UUID版本号（4）和变体（RFC 4122）
        uuid.data[1] = (uuid.data[1] & 0x0FFF) | 0x4000; // 设置版本号为4（0100）
        uuid.data[2] = (uuid.data[2] & 0x3FFF) | 0x8000; // 设置变体为RFC 4122（1000）

        return uuid;
    }

    std::string toString() const {
        std::stringstream ss;
        ss << std::hex << std::setfill('0');
        for (size_t i = 0; i < data.size(); ++i) {
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
    std::array<uint32_t, 4> data{};
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
    using FunctionType = std::function<R(Args...)>;

    explicit Delegate(FunctionType func) : pDelegate(func) {
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

inline static void DoNothing() {
}
#endif //DELEGATE_H
