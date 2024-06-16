#include <iostream>

#include "Delegate.h"
#include "EventManager.h"
#include "TimeTool.h"

int add(int, int) {
    return 0;
}

void TestFunc(std::vector<Event::Object> args) {
    std::cout << "TestFunc2: " << args[0].Cast<int>().value() << std::endl;
}


class Test {
public:
    Test(int);

    int Add(int a, int b) {
        std::cout << "Test::Add" << std::endl;
        return 9;
    }

    void TestFunc(std::vector<Event::Object> args) {
        std::cout << "TestFunc1: " << args[0].Cast<int>().value() << std::endl;
    }
};

enum class Features:unsigned int {
    Feature1 = 1 << 0, // 1
    Feature2 = 1 << 1, // 2
    Feature3 = 1 << 2 // 4
};

void GenerateUUIDs(size_t count) {
    for (size_t i = 0; i < count; i++) {
        Event::UUID::New();
    }
}

int main() {
    auto d = MakeDelegate(TestFunc);
    std::shared_ptr<Test> test;
    auto e = Event::MakeDelegate(&Test::TestFunc, test);
    Event::Delegate<void(std::vector<Event::Object>)> e2 = e;
    std::function f = e2;
    e2({1, 2});
    auto s = Event::Signal::Register("Test");
    s->ModifyDefaultArgs({10086, 2});
    Event::EventManager::AddListener(s, e2);
    e(test, {1, 2});
    s->Trigger();
    return 0;
}
