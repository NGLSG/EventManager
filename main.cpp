#include <iostream>

#include "Delegate.h"
#include "EventManager.h"

int add(int, int) {
    return 0;
}

void TestFunc(std::vector<Event::Object> args) {
    std::cout << "TestFunc" << std::endl;
}


class Test {
public:
    int Add(int a, int b) {
        std::cout << "Test::Add" << std::endl;
        return 9;
    }
};

int main() {
    auto plus = [](int a, int b) { return a + b; };

    auto d = MakeDelegate(TestFunc);
    Test test;
    Event::Delegate<int (Test::*)(int, int)> e = &Test::Add;
    auto s = Event::Signal::Register("Test");
    Event::EventManager::AddListener(s, d);
    s->Trigger();
    e(test, 1, 2);
    return 0;
}
