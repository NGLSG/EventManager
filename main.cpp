#include <utility>
#include "Delegate.h"
#include "EventManager.h"
#include "TimeTool.h"

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

class Foo {
public:
    Foo() = default;

    void func(std::vector<Object>&args) {
        for (size_t i = 1; i < args.size(); ++i) {
            int key = args[i].Cast<int>();
            int j = i - 1;

            // 将元素args[i]插入到已排序的子数组中
            while (j >= 0 && args[j].Cast<int>() > key) {
                args[j + 1] = args[j];
                j--;
            }
            args[j + 1] = key;
        }
    }

    int a = 0;
};

int main() {
    try {
        StopWatch stopwatch;

        auto si = Signal::Register("test");
        Foo foo;

        Calculator calculator;
        Delegate<void(std::vector<Object>)> t1(func);
        Delegate<int (Calculator::*)(int, int)> t2(Calculator::multiply, calculator);

        Delegate<int (Calculator::*)(int, int, int)> t3(Calculator::foo, calculator);
        Delegate<int (int, int)> f2 = t2;
        stopwatch.TestFunctionMultithread(f2, 1000, 6, 6);
        Delegate<void(Calculator::*)(std::vector<Object>)> t4(Calculator::func, calculator);
        EventManager::AddListener(si, t4);
        // 创建一个空的vector来存储随机整数
        std::vector<Object> randomNumbers;

        // 使用std::random_device生成随机数种子
        std::random_device rd;
        std::mt19937 generator(rd());

        // 生成1000个随机整数并添加到vector中
        for (int i = 0; i < 1000; ++i) {
            randomNumbers.push_back(std::uniform_int_distribution<int>(0, 1000)(generator));
        }
        Delegate<void()> t6([&] {
            EventManager::Broadcast(si, randomNumbers);
        });
        std::jthread([&] {
            std::cout << TimeFormat(stopwatch.TestFunctionMultithread(t6, 10)) << std::endl;
        });
        std::jthread([&] {
            std::cout << TimeFormat(stopwatch.TestFunctionMultithread(t4, 10, randomNumbers)) <<
                    std::endl;
        });
    }
    catch (const std::exception&e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
