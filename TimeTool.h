#ifndef TIMETOOL_H
#define TIMETOOL_H
#include <chrono>
#include <future>

#include "Delegate.h"

namespace Event {
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
                future.wait();
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

    inline static std::string TimeFormat(std::chrono::microseconds::rep duration) {
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
}

#endif //TIMETOOL_H
