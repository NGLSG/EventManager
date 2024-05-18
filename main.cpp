#include <iostream>

#include "Delegate.h"
#include "EventManager.h"
#include "TimeTool.h"

int add(int, int) {
    return 0;
}

void TestFunc(std::vector<Event::Object> args) {
    std::cout << "TestFunc2" << std::endl;
}


class Test {
public:
    int Add(int a, int b) {
        std::cout << "Test::Add" << std::endl;
        return 9;
    }

    void TestFunc(std::vector<Event::Object> args) {
        //td::cout << "TestFunc1" << std::endl;
    }
};

int main() {
    auto plus = [](int a, int b) { return a + b; };

    auto d = MakeDelegate(TestFunc);
    std::shared_ptr<Test> test;
    auto e = Event::MakeDelegate(&Test::TestFunc, test);
    Event::Delegate<void(std::vector<Event::Object>)> e2 = e;
    std::function<void(std::vector<Event::Object>)> f = e2;
    std::cout << f.target_type().name() << std::endl;
    e2({1, 2});
    auto s = Event::Signal::Register("Test");
    Event::EventManager::AddListener(s, e2);
    s->Trigger({1, 1});
    e(test, {1, 2});
    return 0;
}
